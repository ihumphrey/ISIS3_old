#ifndef LroNarrowAngleDistortionMap_h
#define LroNarrowAngleDistortionMap_h

#include <vector>
#include "CameraDistortionMap.h"

namespace Isis {
  namespace Lro {

    /** Distort/undistort focal plane coordinates
     *
     * Creates a map for adding/removing optical distortions
     * from the focal plane of a camera.
     *
     * @ingroup Camera
     *
     * @see Camera
     *
     * @author 2009-07-03 Jacob Danton
     * @history 2010-05-10 Ken Edmundson - Corrected computation of distorted
     *          and undistorted locations
     * @history 2010-08-21 Kris Becker - Changed the sign of the distortion 
     *          parameter to match the calibration report.  The LRO/LROC IK
     *          lro_lroc_v14.ti and above contain the appropriate parameters
     *          to coincide with the code change made here.  IMPORTANT:  This
     *          results in Version = 2 of the LroNarrowAngleCamera as depicted
     *          in the Camera.plugin for both NAC-L and NAC-R.
     * @internal
     */
    class LroNarrowAngleDistortionMap : public CameraDistortionMap {
      public:
        LroNarrowAngleDistortionMap(Camera *parent);

        //! Destructor
        virtual ~LroNarrowAngleDistortionMap() {};

        void SetDistortion(const int naifIkCode);

        virtual bool SetFocalPlane(const double dx, const double dy);

        virtual bool SetUndistortedFocalPlane(const double ux, const double uy);

    };
  };
};
#endif
