/************************************************************/
/*    NAME: Vasileios Linardos                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenPath.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "XYPoint.h"    // 4.1
#include "XYSegList.h"  // 4.1
#include "GenPath.h"
#include "ACTable.h"
#include "XYFormatUtilsPoint.h" // 4.1

using namespace std;

//---------------------------------------------------------
// Constructor()

GenPath::GenPath()
{
  m_nav_x = 0; m_nav_y = 0;    //4.1
  m_received_first = false;    //4.1
  m_received_last = false;     //4.1
  m_path_generated = false; 
}
//---------------------------------------------------------
// Destructor

GenPath::~GenPath()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool GenPath::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) 
  {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString(); 
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

    if (key == "VISIT_POINT")                         //4.1 from
    {
      string sval = msg.GetString();
      if (sval == "firstpoint") 
      {
        m_point_pool.clear();
        m_received_first = true;
        m_received_last = false;
        m_path_generated = false;
      } 
      else if (sval == "lastpoint") 
      {
        m_received_last = true;
        //generatePath(); // Παραγωγή διαδρομής μόλις ληφθούν όλα [5]
      } 
      else 
      {
       //XYPoint new_pt;
       //if (new_pt.get_spec(sval)) m_point_pool.push_back(new_pt);   //4.1 set_spec 
       
       XYPoint new_pt = string2Point(sval); 
        
       //double x = tokDoubleParse(sval, "x", ',');
       //double y = tokDoubleParse(sval, "y", ',');
       
       if (new_pt.valid())
       {
         m_point_pool.push_back(new_pt);
       }
      }
    } 
    
    else if (key == "NAV_X") m_nav_x = msg.GetDouble();
    else if (key == "NAV_Y") m_nav_y = msg.GetDouble();    //4.1 to
  
    else if(key == "FOO") 
     cout << "great!";

    else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
      reportRunWarning("Unhandled Mail: " + key);
  }
	
  return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool GenPath::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: generatePath()
//  4.1 all the procedure

void GenPath::generatePath() {
  if (m_point_pool.empty()) return;

  XYSegList tour;
  double cur_x = m_nav_x; // Ξεκινάμε από την τρέχουσα θέση [3]
  double cur_y = m_nav_y;

  vector<XYPoint> points_copy = m_point_pool; // Αντιγραφή για να μην τροποποιηθεί το αρχικό pool

  while (!m_point_pool.empty()) {
    int best_index = -1;
    double min_dist = -1;

    // Εύρεση του πλησιέστερου σημείου (Greedy Algorithm) [3]
    for (unsigned int i=0; i < m_point_pool.size(); i++) {
      double d = hypot(cur_x - m_point_pool[i].x(), cur_y - m_point_pool[i].y());
      if (best_index == -1 || d < min_dist) {
        min_dist = d;
        best_index = i;
      }
    }

    // Προσθήκη στη διαδρομή και ενημέρωση θέσης
    tour.add_vertex(m_point_pool[best_index].x(), m_point_pool[best_index].y());
    cur_x = m_point_pool[best_index].x();
    cur_y = m_point_pool[best_index].y();
    m_point_pool.erase(m_point_pool.begin() + best_index);
  }

  // Αποστολή ενημέρωσης στο Waypoint Behavior του Helm [2, 3, 6]
  string update_str = "points = " + tour.get_spec();
  Notify("GENPATH_UPDATES", update_str); 
}



//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool GenPath::Iterate()
{
  AppCastingMOOSApp::Iterate();
  
  if (m_received_last && !m_path_generated) {
    generatePath(); 
    m_path_generated = true; // Use a member variable to prevent re-generating every 0.25s
  }

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool GenPath::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p=sParams.begin(); p!=sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;
    if(param == "foo") {
      handled = true;
    }
    else if(param == "bar") {
      handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);

  }
  
  Register("VISIT_POINT", 0);     //4.1
  Register("NAV_X", 0);           //4.1
  Register("NAV_Y", 0);          //4.1
  
  registerVariables();	
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void GenPath::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("VISIT_POINT", 0);
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
}


//------------------------------------------------------------
// Procedure: buildReport()

bool GenPath::buildReport() 
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;

  ACTable actab(4);
  actab << "Alpha | Bravo | Charlie | Delta";
  actab.addHeaderLines();
  actab << "one" << "two" << "three" << "four";
  m_msgs << actab.getFormattedString();

  m_msgs << "Points in Pool: " << m_point_pool.size() << endl;           //4.1
 m_msgs << "Path Generated:      " << (m_path_generated ? "yes" : "no") << endl;
  m_msgs << "Last Point Received: " << (m_received_last ? "yes" : "no") << endl;
  
  return(true);
}




