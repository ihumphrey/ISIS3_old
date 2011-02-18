#include <iostream>
#include <iomanip>
#include "SpicePosition.h"
#include "Filename.h"
#include "Preference.h"
#include "Table.h"

using namespace std;

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  cout << setprecision(10);
  cout << "Unit test for SpicePosition" << endl;

  Isis::Filename f("$base/testData/kernels");
  string dir = f.Expanded() + "/";
  string moc(dir + "moc.bsp");
  string de(dir + "de405.bsp");
  string pck(dir + "pck00006.tpc");
  furnsh_c(moc.c_str());
  furnsh_c(de.c_str());
  furnsh_c(pck.c_str());

  double startTime = -69382819.0;
  double endTime = -69382512.0;
  double slope = (endTime - startTime) / (10 - 1);

  Isis::SpicePosition pos(-94, 499);

  // Normal testing (no cache)
  cout << "Testing without cache ... " << endl;
  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    vector<double> p = pos.SetEphemerisTime(t);
    vector<double> v = pos.Velocity();
    cout << "Time           = " << pos.EphemerisTime() << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;


  // Testing with cache
  cout << "Testing with cache ... " << endl;
  pos.LoadCache(startTime, endTime, 10);
  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    vector<double> p = pos.SetEphemerisTime(t);
    vector<double> v = pos.Velocity();
    cout << "Time           = " << pos.EphemerisTime() << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;

  // Test table options
  cout << "Testing tables ... " << endl;
  Isis::Table tab = pos.Cache("Test");
  Isis::SpicePosition pos2(-94, 499);
  pos2.LoadCache(tab);
  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    pos2.SetEphemerisTime(t);
    vector<double> p = pos2.Coordinate();
    vector<double> v = pos2.Velocity();
    cout << "Time           = " << pos2.EphemerisTime() << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;

  // Test polynomial functions
  cout << "Testing with polynomial functions..." << endl;
  std::vector<double> abcPos1, abcPos2, abcPos3;
  //  pos.SetOverrideBaseTime(0.,1.);
  pos.ComputeBaseTime();
  pos.SetPolynomialDegree(7);
  pos.SetPolynomial();
  pos.GetPolynomial(abcPos1, abcPos2, abcPos3);
  //  cout << "Source = " << pos.GetSource() << endl;

  for (int i=0; i<10; i++) {
    double t = startTime + (double) i * slope;
    pos.SetEphemerisTime(t);
    vector<double> p = pos.Coordinate();
    vector<double> v = pos.Velocity();
    cout << "Time           = " << pos.EphemerisTime() << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;

  // Now use LineCache method 
  cout << "Testing line cache..." << endl;
  tab = pos.LineCache("Test2");
  Isis::SpicePosition pos3(-94, 499);
  pos3.LoadCache(tab);

  for (int i=0; i<10; i++) {
    double t = startTime + (double) i * slope;
    pos3.SetEphemerisTime(t);
    vector<double> p = pos3.Coordinate();
    vector<double> v = pos3.Velocity();
    cout << "Time           = " << pos.EphemerisTime() << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;
 

  // Also test Extrapolate method
  cout << "Testing extrapolation..." << std::endl;
  pos3.SetEphemerisTime(endTime);
  cout << "Time           = " << pos3.EphemerisTime() << endl;
  vector<double> p = pos3.Coordinate();
  cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
  cout << "Time           = " << pos3.EphemerisTime()+.000001 << endl;
  p = pos3.Extrapolate(endTime+.000001);
  cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
  cout << endl;
  
}
