/************************************************************/
/*    NAME: Vasileios Linardos                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PointAssign.h                                          */
/*    DATE: June 18th, 2026                             */
/************************************************************/

#ifndef PointAssign_HEADER
#define PointAssign_HEADER

#include <vector>
#include <string>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include "XYPoint.h"
#include "XYFormatUtilsPoint.h"

class PointAssign : public AppCastingMOOSApp
{
 public:
   PointAssign();    //{m_assign_by_region=false; m_point_count=0;};
   ~PointAssign();

 protected: // Standard MOOSApp functions to overload  
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected: // Standard AppCastingMOOSApp function to overload 
   bool buildReport();
   void postViewPoint(double x, double y, std::string label, std::string color); //final

 protected:
  

 private: // Configuration variables
  void registerVariables();

 private: // State variables
  std::vector<std::string> m_vnames;
  bool   m_assign_by_region;
  int    m_point_count;
};

#endif 
