/************************************************************/
/*    NAME: Vasileios Linardos                              */
/*    ORGN: MIT                                             */
/*    FILE: GenRescue.cpp                                   */
/*    DATE: June 23th, 2026                                 */
/************************************************************/

#include <iterator>
#include "GenRescue.h"
#include "MBUtils.h"
#include "ColorParse.h"
#include "XYPoint.h"
#include "XYSegList.h"
#include "GeomUtils.h"
#include "PathUtils.h"
#include "XYFormatUtilsPoly.h"
#include "XYFieldGenerator.h"
#include <vector>  //
#include <cmath>   //
#include <float.h>  //
#include "NodeRecord.h"
#include "NodeRecordUtils.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

GenRescue::GenRescue()
{
  // Initialize state variables
  m_nav_x = 0;
  m_nav_y = 0;
  m_nav_x_set = 0;
  m_nav_y_set = 0;
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool GenRescue::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key  = msg.GetKey();
    string sval = msg.GetString();

    bool handled = true;
    if(key == "SWIMMER_ALERT") 
      handled = handleMailNewSwimmer(sval);
    else if(key == "FOUND_SWIMMER") 
      handled = handleMailFoundSwimmer(sval);
    else if(key == "NAV_X") {
      m_nav_x = msg.GetDouble();
      m_nav_x_set = true;
    }
    else if(key == "NAV_Y") {
      m_nav_y = msg.GetDouble();
      m_nav_y_set = true;
    }
    
    else if (key == "NODE_REPORT") 
    {
     NodeRecord record = string2NodeRecord(msg.GetString());
     m_adv_x = record.getX();
     m_adv_y = record.getY();
     m_adv_hdg = record.getHeading();
     m_adv_set = true;
    }

    else if(key != "APPCAST_REQ") // handle by AppCastingMOOSApp
      handled = false;
    
    if(!handled)
      reportRunWarning("Unhandled Mail: " + key +"=" + sval);
    
  }
  return(true);
}
 
//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool GenRescue::OnConnectToServer()
{
  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()

bool GenRescue::Iterate()
{
  AppCastingMOOSApp::Iterate();
  
  //if(m_plan_pending)
  if((m_iteration % 20) == 0)
  //  postShortestPath();

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()

bool GenRescue::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp(); 

  STRING_LIST sParams;
  m_MissionReader.GetConfiguration(GetAppName(), sParams);
  
  STRING_LIST::iterator p;
  for(p=sParams.begin(); p!=sParams.end(); p++) {
    string sLine  = *p;
    string param  = tolower(biteStringX(sLine, '='));
    string value  = sLine;
    if(param == "vname")
      m_vname = value;
  }
  
  Register("NODE_REPORT", 0);
  
  RegisterVariables();	
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables()

void GenRescue::RegisterVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("SWIMMER_ALERT", 0);
  Register("FOUND_SWIMMER", 0);
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
}


//---------------------------------------------------------
// Procedure: handleMailNewSwimmer()

bool GenRescue::handleMailNewSwimmer(string str)
{
  string sx  = tokStringParse(str, "x", ',', '=');
  string sy  = tokStringParse(str, "y", ',', '=');
  string sid = tokStringParse(str, "id", ',', '=');

  if((sx == "") || (sy == "") || (sid == ""))
    return(false);

  double x = atof(sx.c_str());
  double y = atof(sy.c_str());

  // Ignore swimmers we already know about.
  // Ignore swimmers already rescued/found.
  if(m_found_ids.count(sid) != 0)
    return(true);

  // Ignore swimmers we already know about.
  if(m_map_pts.count(sid) != 0)
    return(true);

  XYPoint pt(x, y);
  pt.set_label(sid);

  m_map_pts[sid] = pt;

  // Force the path to be rebuilt using the new swimmer list.
  m_path.clear();

  postShortestPath();

  reportEvent("New swimmer alert: id=" + sid + ", x=" + sx + ", y=" + sy);
  return(true);
}

//---------------------------------------------------------
// Procedure: handleMailFoundSwimmer()

bool GenRescue::handleMailFoundSwimmer(string str)
{
  string sid = tokStringParse(str, "id", ',', '=');

  if(sid == "")
    return(false);

  m_found_ids.insert(sid);

  if(m_map_pts.count(sid) != 0)
    m_map_pts.erase(sid);

  m_path.clear();
  postShortestPath();

  reportEvent("Found swimmer removed from path: id=" + sid);
  return(true);
}
//---------------------------------------------------------
// Procedure: postShortestPath()

void GenRescue::postShortestPath()
{
  // if(m_map_pts.size() == 0)
  //   return;

  // XYSegList new_path;

  // map<string, XYPoint>::iterator p;
  // for(p = m_map_pts.begin(); p != m_map_pts.end(); p++) {
  //   XYPoint pt = p->second;
  //   new_path.add_vertex(pt.x(), pt.y());
  // }

  // if(m_nav_x_set && m_nav_y_set)
  //   new_path = greedyPath(new_path, m_nav_x, m_nav_y);
  // else
  //   new_path = greedyPath(new_path, 0, 0);

  if(m_map_pts.empty())
    return;

  // 1. Gather all current unvisited points from the map into a vector for easier processing
  std::vector<XYPoint> unvisited;
  std::map<std::string, XYPoint>::iterator p;
  for(p = m_map_pts.begin(); p != m_map_pts.end(); p++) {
   XYPoint pt = p->second;
    bool concede = false;

    if (m_adv_set) {
      double dist_to_adv = hypot(pt.x() - m_adv_x, pt.y() - m_adv_y);
      if (dist_to_adv < 15.0) 
        concede = true; // Strategy: Concede swimmers closest to adversary [2]
    }

    if (!concede)
      unvisited.push_back(pt);

  }

  // 2. Determine starting position
  double cur_x = 0;
  double cur_y = 0;
  if(m_nav_x_set && m_nav_y_set) {
    cur_x = m_nav_x;
    cur_y = m_nav_y;
  }

  // 3. Generate the path using the two-vertex-look-ahead algorithm
  XYSegList new_path;
  
  while(!unvisited.empty()) {
    int best_index = -1;
    double min_total_score = -1;

    for(unsigned int i=0; i<unvisited.size(); i++) {
      // Leg 1: Distance from vehicle's current/last point to candidate A
      double d1 = hypot(cur_x - unvisited[i].x(), cur_y - unvisited[i].y());
      
      // Leg 2: Distance from candidate A to its nearest unvisited neighbor B
      double d2 = 0;
      bool found_neighbor = false;
      for(unsigned int j=0; j<unvisited.size(); j++) {
        if(i == j) continue;
        double dist_to_neighbor = hypot(unvisited[i].x() - unvisited[j].x(), 
                                        unvisited[i].y() - unvisited[j].y());
        if(!found_neighbor || dist_to_neighbor < d2) {
          
          d2 = dist_to_neighbor;
          found_neighbor = true;
        }
      }
      
      // Two-vertex evaluation score
      double total_score = d1 + d2;

      if(best_index == -1 || total_score < min_total_score) {
        min_total_score = total_score;
        best_index = i;
      }
    }

    // Add the best candidate found to the path
    if(best_index != -1) {
      new_path.add_vertex(unvisited[best_index].x(), unvisited[best_index].y());
      cur_x = unvisited[best_index].x();
      cur_y = unvisited[best_index].y();
      unvisited.erase(unvisited.begin() + best_index);
    }
  }


// Koino Meros toy Kvdika

  new_path.set_label("rescue_path");

  m_path = new_path;

  Notify("VIEW_SEGLIST", m_path.get_spec());
 
  
  
  //string update_var = "SURVEY_UPDATE";
  string update_str = "points = " + m_path.get_spec_pts();
Notify("SURVEY_UPDATE", update_str);
  //Notify(update_var, update_str);
  reportEvent("SURVEY_UPDATE=" + update_str);



}

//---------------------------------------------------------
// Procedure: postNullPath()
//   Purpose: If a found swimmer represents the last swimmer
//            to be found, then post a survey update essentially
// 

void GenRescue::postNullPath()
{
#if 0
  if(!m_nav_x_set || !m_nav_y_set)
    return;
  if(m_map_pts.size() != 0)
    return;
  
  XYSegList segl;
  segl.add_vertex(m_nav_x, m_nav_y);
  
  // Seglist needs a name, refer when drawging and erasing
  segl.set_label("one");
  Notify("VIEW_SEGLIST", segl.get_spec());

  string update_var = "SURVEY_UPDATE";
  string update_str = "points = " + segl.get_spec_pts();

  Notify(update_var, update_str);
  reportEvent("SURVEY_UPDATE=" + update_str);
#endif
}


//---------------------------------------------------------
// Procedure: buildReport()

bool GenRescue::buildReport()
{
  return(true);
}
