/************************************************************/
/*    NAME: Jane Doe                                        */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: Odometry.cpp                                    */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "MBUtils.h"
#include "ACTable.h"
#include "Odometry.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

Odometry::Odometry()
{
  m_first_reading = true;

  m_current_x = 0;
  m_current_y = 0;
  m_previous_x = 0;
  m_previous_y = 0;

  m_total_distance = 0;
  m_depth_distance = 0;
  m_current_depth = 0;

  m_depth_thresh = 0;

  m_last_nav_time = 0;
  m_nav_warning_posted = false;
}

//---------------------------------------------------------
// Destructor

Odometry::~Odometry()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool Odometry::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p = NewMail.begin(); p != NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();

    bool nav_update = false;

    if(key == "NAV_X") {
      m_current_x = msg.GetDouble();
      cout << "nav_x: " << m_current_x << endl;
      nav_update = true;
    }
    else if(key == "NAV_Y") {
      m_current_y = msg.GetDouble();
      cout << "nav_y: " << m_current_y << endl;
      nav_update = true;
    }
    else if(key == "NAV_DEPTH") {
      m_current_depth = msg.GetDouble();
    }
    else if(key != "APPCAST_REQ") {
      reportRunWarning("Unhandled Mail: " + key);
    }

    if(nav_update) {
      m_last_nav_time = MOOSTime();
      retractRunWarning("No recent NAV_X or NAV_Y received");
      m_nav_warning_posted = false;

      if(m_first_reading) {
        m_previous_x = m_current_x;
        m_previous_y = m_current_y;
        m_first_reading = false;
      }
      else {
        double delta_x = m_current_x - m_previous_x;
        double delta_y = m_current_y - m_previous_y;
        double dist = sqrt((delta_x * delta_x) + (delta_y * delta_y));

        Notify("LEG_DIST", dist);

        m_total_distance += dist;

        if(m_current_depth > m_depth_thresh)
          m_depth_distance += dist;

        cout << "ODOMETRY_DIST: " << m_total_distance << endl;
        cout << "ODOMETRY_DIST_AT_DEPTH: " << m_depth_distance << endl;

        m_previous_x = m_current_x;
        m_previous_y = m_current_y;
      }
    }
  }

  return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool Odometry::OnConnectToServer()
{
  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool Odometry::Iterate()
{
  AppCastingMOOSApp::Iterate();

  double elapsed = MOOSTime() - m_last_nav_time;

  if((!m_first_reading) && (elapsed >= 10.0) && (!m_nav_warning_posted)) {
    reportRunWarning("No recent NAV_X or NAV_Y received");
    m_nav_warning_posted = true;
  }

  Notify("ODOMETRY_DIST", m_total_distance);
  Notify("ODOMETRY_DIST_AT_DEPTH", m_depth_distance);

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool Odometry::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);

  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p = sParams.begin(); p != sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;

    if(param == "depth_thresh") {
      handled = true;

      if(isNumber(value) && (atof(value.c_str()) >= 0))
        m_depth_thresh = atof(value.c_str());
      else
        reportConfigWarning("depth_thresh must be a number greater than or equal to zero");
    }
    else if(param == "foo") {
      handled = true;
    }
    else if(param == "bar") {
      handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void Odometry::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();

  Register("NAV_X", 0);
  Register("NAV_Y", 0);
  Register("NAV_DEPTH", 0);
}

//------------------------------------------------------------
// Procedure: buildReport()

bool Odometry::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "Odometry Report" << endl;
  m_msgs << "============================================" << endl;
  m_msgs << "Total odometry distance:      " << m_total_distance << endl;
  m_msgs << "Depth threshold:              " << m_depth_thresh << endl;
  m_msgs << "Distance meeting threshold:   " << m_depth_distance << endl;
  m_msgs << "Current depth:                " << m_current_depth << endl;

  return(true);
}