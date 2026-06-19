/************************************************************/
/*    NAME: Vasileios Linardos                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PointAssign.cpp                                 */
/*    DATE: June 18th, 2026                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "XYPoint.h"
#include "PointAssign.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

PointAssign::PointAssign()
{
  m_assign_by_region = false;
  m_point_count = 0;
}

//---------------------------------------------------------
// Destructor

PointAssign::~PointAssign()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool PointAssign::OnNewMail(MOOSMSG_LIST &NewMail)
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

    if (key == "VISIT_POINT")            //
    {                                    //
      string sval = msg.GetString();     //

      // 1. Χειρισμός Bookend Μηνυμάτων [4, 5]
      if (sval == "firstpoint" || sval == "lastpoint")    //
      {                                                   //
        for (string vname : m_vnames)                     //
        {                                                 //
         Notify("VISIT_POINT_" + vname, sval);            //
        }                                                 //
      }                                                   //
      // 2. Χειρισμός Δεδομένων Συντεταγμένων
      else                                                //
      {                                                   //
        unsigned int index = 0;                           //
        if (m_assign_by_region)                           //
        {                                                 //
          // Διανομή βάσει περιοχής: Το μέσο X είναι 87.5 για το εύρος -25 έως 200 [5, 8]
          XYPoint point;                                  //
          point.get_spec(sval);                           //set.spec
          index = (point.get_vx() < 87.5) ? 0 : 1;         //get_x
        }                                                 //
        else                                              //
        {                                                 //
          // Εναλλασσόμενη διανομή (Alternating) για 50-50 μοίρασμα [5]
          index = m_point_count % m_vnames.size();        //
        }                                                 //

        if (index < m_vnames.size())                      //
        {                                                 //
          Notify("VISIT_POINT_" + m_vnames[index], sval);    //
          m_point_count++;                                 //
        }                                                 //
      }                                                   //
    }                                                     //

    if(key == "FOO") 
      cout << "great!";

    else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
      reportRunWarning("Unhandled Mail: " + key);
  }
	
  return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool PointAssign::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool PointAssign::Iterate()
{
  AppCastingMOOSApp::Iterate();
  // Do your thing here!
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool PointAssign::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  m_MissionReader.GetConfiguration(GetAppName(), sParams);     //
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p=sParams.begin(); p!=sParams.end(); p++) {
 // for(std::string line : sParams) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    if (param == "vname") 
    {         //
      m_vnames.push_back(toupper(value)); // Προσθήκη HENRY, GILDA [5]
    } 
    else if (param == "assign_by_region") 
    {     //
      m_assign_by_region = (tolower(value) == "true");   //
    }    //

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
  //Register("VISIT_POINT", 0);        //enter in final
  registerVariables();	
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void PointAssign::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("VISIT_POINT", 0);        //
}


//---------------------------------------------------------
// Procedure: Variables compantible to pMarineViewer

//void PointAssign::postViewPoint(double x, double y, string label, string color) {
 // XYPoint point(x, y);
 // point.set_label(label);          // Χρήση του ID του σημείου ως μοναδικό label [2]
 // point.set_color("vertex", color); // Επιλογή χρώματος ανά όχημα για διάκριση [2]
  //point.set_param("vertex_size", "4");

 // string spec = point.get_spec();   // Μετατροπή σε string spec [2]
 // Notify("VIEW_POINT", spec);       // Δημοσίευση στο MOOSDB [2]
//}

//------------------------------------------------------------
// Procedure: buildReport()

bool PointAssign::buildReport() 
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;

  ACTable actab(4);
  actab << "Alpha | Bravo | Charlie | Delta";
  actab.addHeaderLines();
  actab << "one" << "two" << "three" << "four";
  m_msgs << actab.getFormattedString();
  m_msgs << "Registered Vehicles: " << m_vnames.size() << endl;  //
  m_msgs << "Points Distributed: " << m_point_count << endl;     //
  m_msgs << "Mode: " << (m_assign_by_region ? "Region" : "Alternating") << endl;   //

  return(true);
}




