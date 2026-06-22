/************************************************************/
/*    NAME: Jane Doe                                        */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: Odometry.h                                      */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef Odometry_HEADER
#define Odometry_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class Odometry : public AppCastingMOOSApp
{
 public:
   Odometry();
   ~Odometry();

 protected: // Standard MOOSApp functions to overload
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected: // Standard AppCastingMOOSApp function to overload
   bool buildReport();

 protected:
   void registerVariables();

 private: // Configuration variables
   double m_depth_thresh;

 private: // State variables
   bool   m_first_reading;

   double m_current_x;
   double m_current_y;
   double m_previous_x;
   double m_previous_y;

   double m_total_distance;
   double m_depth_distance;
   double m_current_depth;

   double m_last_nav_time;
   bool   m_nav_warning_posted;
};

#endif