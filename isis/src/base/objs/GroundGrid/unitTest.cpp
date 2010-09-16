#include "Isis.h"

#include <iostream>

#include "Cube.h"
#include "UniversalGroundMap.h"
#include "GroundGrid.h"
#include "Progress.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  /*
    The output of this class directly correlates to the output of the "grid"
    application when mode=ground - if that application is correct, then this
    object is correct and vice versa.
   */
  Cube someCube;

  cout << "Reading cube..." << endl;
  someCube.Open("$mgs/testData/ab102401.cub");

  cout << "Create universal ground map..." << endl;
  UniversalGroundMap gmap(someCube);

  cout << "Create grid..." << endl;
  Progress progress;
  GroundGrid grid(&gmap, false, someCube.Samples(), someCube.Lines());

  grid.CreateGrid(0, 0, 0.2, 0.2, &progress, 0.1, 0.01);

  cout << "\n\nGrid cutout: \n" << endl;

  for(int line = 0; line < someCube.Lines() / 4; line++) {

    for(int i = (int)(someCube.Samples() * 3.0 / 7.0);
            i < (someCube.Samples() * 3.5 / 7.0); i++) {
      cout << grid.PixelOnGrid(i, line);
    }
    
    cout << endl;
  }

  cout << endl;
}
