/**
 * @file
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for 
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software 
 *   and related material nor shall the fact of distribution constitute any such 
 *   warranty, and no responsibility is assumed by the USGS in connection 
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see 
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 *
 * @author 2003-03-13 Jeff Anderson?
 *
 * @internal
 *  @history 2012-10-11 Debbie A. Cook - Updated to use new Target and Shape Model classes.
 */
#include <iostream>
#include <iomanip>

#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Spice.h"
#include "Preference.h"
#include "Target.h"

using namespace Isis;
using namespace std;

/**
 * UnitTest for Spice class.
 *
 * @internal
 * @history 2009-03-23  Tracie Sucharski - Removed old keywords
 *          SpacecraftPosition and SpacecraftPointing with the corrected
 *          InstrumentPosition and InstrumentPointing.
 */
class MySpice : public Spice {
  public:
    MySpice(Pvl &lab) : Spice(lab) {
      cout << "BodyCode        = " << naifBodyCode() << endl;
      cout << "SpkCode         = " << naifSpkCode() << endl;
      cout << "CkCode          = " << naifCkCode() << endl;
      cout << "IkCode          = " << naifIkCode() << endl;
      cout << endl;
    }

    int MyInteger(string key) {
      return getInteger(key, 0);
    }

    double MyDouble(string key) {
      return getDouble(key, 0);
    }

    string MyString(string key) {
      return getString(key, 0);
    }

    void MyOutput() {
      cout << "BJ is " << endl;
      vector<double> BJ = bodyRotation()->Matrix();
      for(int i = 0; i < (int)BJ.size(); i++) {
        cout << BJ[i] << endl;
      }
      vector<double> IJ = instrumentRotation()->Matrix();
      vector<double> BI = IJ;
      mxmt_c((SpiceDouble( *)[3])&BJ[0], (SpiceDouble( *)[3])&IJ[0],
             (SpiceDouble( *)[3])&BI[0]);

      cout << "BP is " << endl;
      for(int i = 0; i < (int)BI.size(); i++) {
        cout << BI[i] << endl;
      }
    }

    double Resolution() {
      return 1.;
    }
};



int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << setprecision(10);
  cout << "Unit test for Isis::Spice" << endl;

  PvlGroup inst("Instrument");
  inst += PvlKeyword("TargetName", "Mard");

  PvlGroup kern("Kernels");
  FileName f("$base/testData/kernels");
  string dir = f.expanded() + "/";
  kern += PvlKeyword("NaifFrameCode", -94031);
  kern += PvlKeyword("LeapSecond", dir + "naif0007.tls");
  kern += PvlKeyword("SpacecraftClock", dir + "MGS_SCLKSCET.00045.tsc");
  kern += PvlKeyword("TargetPosition", dir + "de405.bsp");
  kern += PvlKeyword("TargetAttitudeShape", dir + "pck00006.tpc");
  kern += PvlKeyword("Instrument", dir + "mocSpiceUnitTest.ti");
  kern += PvlKeyword("InstrumentAddendum", dir + "mocAddendum.ti");
  kern += PvlKeyword("InstrumentPosition", dir + "moc.bsp");
  kern += PvlKeyword("InstrumentPointing", dir + "moc.bc");
  kern += PvlKeyword("Frame", "");

  // Time Setup
  double startTime = -69382819.0;
  double endTime = -69382512.0;
  double slope = (endTime - startTime) / (10 - 1);

  kern += PvlKeyword("StartPadding", slope);
  kern += PvlKeyword("EndPadding", slope);

  Pvl lab;
  lab.AddGroup(inst);
  lab.AddGroup(kern);

  // Test bad target
  try {
    cout << "Testing unknown target ..." << endl;
    MySpice spi(lab);
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }

  // Test bad getInteger
  PvlGroup &temp = lab.FindGroup("Instrument");
  temp.AddKeyword(PvlKeyword("TargetName", "Mars"), Pvl::Replace);
  cout << "Creating Spice object ..." << endl;
  MySpice spi(lab);
  spi.instrumentRotation()->SetTimeBias(-1.15);
  cout << endl;

  try {
    cout << "Testing unknown integer keyword ... " << endl;
    spi.MyInteger("BadInteger");
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }

  // Test bad getDouble
  try {
    cout << "Testing unknown double keyword ... " << endl;
    spi.MyDouble("BadDouble");
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }

  // Test bad getString
  try {
    cout << "Testing unknown string keyword ... " << endl;
    spi.MyString("BadString");
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }

  // Test good gets
  cout << "Testing convience get methods ... " << endl;
  cout << spi.MyInteger("FRAME_MGS_MOC") << endl;
  cout << spi.MyDouble("INS-94030_NA_FOCAL_LENGTH") << endl;
  cout << spi.MyString("FRAME_-94031_NAME") << endl;
  cout << endl;

  // Testing radius
  cout << "Testing radius ... " << endl;
  Distance radii[3];
  spi.radii(radii);
  cout << "Radii[0]:  " << radii[0].kilometers() << endl;
  cout << "Radii[1]:  " << radii[1].kilometers() << endl;
  cout << "Radii[2]:  " << radii[2].kilometers() << endl;
  cout << endl;

  // Normal testing (no cache)
  cout << "Testing without cache ... " << endl;
  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    spi.setTime(t);
    cout << "Time           = " << spi.time().Et() << endl;
    double p[3];
    spi.instrumentPosition(p);
    cout << "Spacecraft (B) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    double v[3];
    spi.instrumentVelocity(v);
    cout << "Spacecraft Velocity (B) = " << v[0] << " " << v[1] << " " << v[2] << endl;
    spi.sunPosition(p);
    cout << "Sun        (B) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    spi.MyOutput();
    double lat, lon;
    spi.subSpacecraftPoint(lat, lon);
    cout << "SubSpacecraft  = " << lat << " " << lon << endl;
    spi.subSolarPoint(lat, lon);
    cout << "SubSolar       = " << lat << " " << lon << endl;
  }
  cout << endl;

  // Testing with cache
  cout << "Testing with cache ... " << endl;
  double tol = .0022; //estimate resolution pixelPitch*alt/fl*1000.
  spi.createCache(startTime + slope, endTime - slope, 10, tol);
  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    spi.setTime(t);
    cout << "Time           = " << spi.time().Et() << endl;
    double p[3];
    spi.instrumentPosition(p);
    cout << "Spacecraft (B) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    double v[3];
    spi.instrumentVelocity(v);
    cout << "Spacecraft Velocity (B) = " << v[0] << " " << v[1] << " " << v[2] << endl;
    spi.sunPosition(p);
    cout << "Sun        (B) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    spi.MyOutput();
  }
  cout << endl;

  cout << "Testing Utility methods" << endl;
  cout << "Target Name = " << spi.target()->name() << endl;
}
