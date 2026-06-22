/************************************************************/
/*    NAME: Vasileios Linardos                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenPath.h                                       */
/*    DATE: June 18th, 2026                                 */
/************************************************************/

#ifndef GenPath_HEADER
#define GenPath_HEADER

#include <vector>  //4.1
#include <string>  //4.1
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include "XYPoint.h"     //4.1
#include "XYSegList.h"   //4.1


class GenPath : public AppCastingMOOSApp
{
 public:
   GenPath();
   ~GenPath();

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
   void generatePath();     //4.1

 private: // State variables
   std::vector<XYPoint> m_point_pool;  //4.1
   double m_nav_x;                    //4.1
   double m_nav_y;                    //4.1
   bool   m_received_first;           //4.1
   bool   m_received_last;            //4.1
   bool   m_path_generated;
};

   //bool buildReport();

#endif 
