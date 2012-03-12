#include "Isis.h"
#include "Cube.h"
#include "Table.h"
#include "Camera.h"
#include "IException.h"
#include "CameraDetectorMap.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  try {
    // Open the cube
    Cube cube;
    cube.open(ui.GetFilename("FROM"), "rw");

    //check for existing polygon, if exists delete it
    if(cube.getLabel()->HasObject("Polygon")) {
      cube.getLabel()->DeleteObject("Polygon");
    }

    // Get the camera, interpolate to a parabola
    Camera *cam = cube.getCamera();
    if(cam->DetectorMap()->LineRate() == 0.0) {
      string msg = "[" + ui.GetFilename("FROM") + "] is not a line scan camera";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    cam->InstrumentRotation()->SetPolynomial();

    // Get the instrument pointing keyword from the kernels group and update
    // its value to table.
    Isis::PvlGroup kernels =
      cube.getLabel()->FindGroup("Kernels", Isis::Pvl::Traverse);

    // Write out the "Table" label to the tabled kernels in the kernels group
    kernels["InstrumentPointing"] = "Table";
    cube.putGroup(kernels);

    // Pull out the pointing cache as a table and write it
    Table cmatrix = cam->InstrumentRotation()->Cache("InstrumentPointing");
    cmatrix.Label().AddComment("Smoothed using spicefit");
    cube.write(cmatrix);
    cube.close();
  }
  catch(IException &e) {
    string msg = "Unable to fit pointing for [" + ui.GetFilename("FROM") + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }
}
