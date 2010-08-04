#ifndef AutoReg_h
#define AutoReg_h
/**
 * @file
 * $Revision: 1.27 $
 * $Date: 2010/06/15 19:37:51 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */


#include <string>
#include <vector>

#include "Chip.h"
#include "Statistics.h"


using namespace std;
namespace Isis {
  /**
   * @brief Auto Registration class
   *
   * Create AutoReg object.  Because this is a pure virtual class you can
   * not create an AutoReg class directly.  Instead, see the AutoRegFactory
   * class.
   *
   * @ingroup PatternMatching
   *
   * @see AutoRegFactory MaximumCorrelation MinimumDifference
   *
   * @internal
   *   @history 2006-01-11 Jacob Danton Added idealFit variable, option for sub-pixel
   *                                     accuracy, new CompareFit method, an arbitrarily
   *                                     chosen EPSILON value, a minimum standard deviation
   *                                     cutoff for the pattern chip, and created a unitTest
   *   @history 2006-02-13 Jacob Danton Added shrinking pattern and sampling option for the pattern chip.
   *   @history 2006-05-15 Jeff Anderson Moved ZScoreMinimum from
   *                                     Algorithm group to the
   *                                     PatternChip group
   *   @history 2006-05-22 Jacob Danton Added statistics reporting
   *   @history 2006-06-28 Brendan George Added copy constructor
   *            (private member function)
   *   @history 2008-06-23 Steven Lambright Fixed naming and some documentation
   *            fixes, added ZScores
   *   @history 2008-08-13 Noah Hilt Added two new optional arguments to AutoReg: WindowSize and
        DistanceTolerance. These two arguments affect how AutoReg gathers and
   *    compares data for surface modeling and accuracy and should be contained
   *    inside the group "SurfaceModel" in the Pvl. Also changed the Pvl returned
   *    by RegistrationStatistics so that group names do not contain spaces and
   *    the Pvl only contains groups.
   *    @history 2008-11-18 Steven Koechle - Changed all keywords *NotEnoughData
   *             to *NotEnoughValidData
   *    @history 2009-03-17  Tracie Sucharski - Added method to return fit chip
   *    @history 2009-03-26  Jeff Anderson - Removed pattern chip
   *             reduction as it was broken, added skewness check
   *             for the fit chip, cleaned up the
   *             RegistrationStatistics method and added setters
   *             methods where appropriate
   *    @history 2009-03-30 Jeff Anderson - Modified code to reset
   *             parameters (goodness of fit and skewness) for
   *             successive calls to Register method.  Also check
   *             to see if skewness is null.
   *    @history 2009-05-08 Stacy Alley - Took out the skewness
   *             test and added the ellipse eccentricity test in
   *             the ModelSurface method.  Also added the 'reduce'
   *             option to speed up the pattern matching process.
   *    @history 2009-06-02 Stacy Alley - ModelSurface method now
   *             returns a bool instead of an int.  The p_status
   *             is set within this method now. The Match method
   *             now takes another arg... fChip, passed in from
   *             Register. Also took out a redundant test,
   *             'CompareFits' after the ModelSurface call.  Also
   *             changed all the Chips in this header file from
   *             pointers to non-pointers.
   *             Saved all the reduced chips and have
   *             methods to return them so they can be views from
   *             Qnet.
   *    @history 2009-06-02 Jeff Anderson - Added
   *             AdaptiveRegistration virtual methods
   *    @history 2009-08-07 Travis Addair - Added functionality
   *             allowing it and all its sublcasses to return
   *             their auto registration template parameters
   *    @history 2009-08-25 Kris Becker - Correct calls to abs instead of the
   *             proper fabs.
   *    @history 2009-08-26 Kris Becker - Added chip parameters to Adaptive
   *             method to coorespond to the Match method.  Also added best
   *             search sample and line parameters to Adaptive method.  Also
   *             needed way to relate best fit value.
   *    @history 2009-09-02 Kris Becker - Set the default valid minimum and
   *             maximum values for pattern and search chips to
   *             Isis::ValidMinimum and Isis::ValidMaximum, respectively, as
   *             opposed to -DBL_MAX and _DBL_MAX.  This modification has the
   *             net effect of excluding special pixels from a valid test.  Also
   *             fix a bug whereby the extracted subsearch chip valid percent
   *             was divided by 100 - it instead should be passed in as is with
   *             the pattern chip test.
   *    @history 2009-09-07 Kris Becker - Set the goodness of fit
   *             when successful Adaptive algorithm.
   *    @history 2010-02-22 Tracie Sucharski - Added return
   *             methods for settings.
   *    @history 2010-03-16 Travis Addair - Added option to skip
   *             eccentricity testing, and made it so that the
   *             eccentricity is assumed to be 0 when it cannot
   *             otherwise be computed.
   *    @history 2010-03-18 Travis Addair - Added optional surface
   *             model validity test to ensure that the average
   *             residual of the least squares solution is within
   *             a tolerance.
   *    @history 2010-03-19 Travis Addair - Changed eccentricity
   *             and residual tests to be disabled by default.
   *             Including the keyword EccentricityRatio or
   *             ResidualTolerance in the Pvl that constructs an
   *             AutoReg object will enable the respective test.
   *    @history 2010-03-26 Travis Addair - Added methods
   *             Eccentricity() and AverageResidual() to retrieve
   *             the last computed value for those variables
   *    @history 2010-04-08 Travis Addair - Added methods
   *             EccentricityRatio() and
   *             EccentricityRatioTolerance() to retrieve  an
   *             eccentricity value as the antecedent in an A:1
   *             ratio.
   *    @history 2010-06-15 Jeannie Walldren - Modified code to read in an
   *             Interpolator type from the pvl and set the pattern and search
   *             chips' Read() methods to use this Interpolator type. Updated
   *             documentation and unitTest.
   *    @history 2010-07-09 Travis Addair - ComputeChipZScore now requires that
   *             both p_ZScoreMin and p_ZScoreMax are less than the pattern
   *             stats minimum z-score in order to fail.
   *    @history 2010-07-20 Jeannie Walldren - Added ability to set a valid
   *             percentage for the search chip's subchip. Updated documentation
   *             and unitTest.
   *    @history 2010-08-04 Jeannie Walldren - Updated documentation.
   *
   */
  class Pvl;
  class AutoRegItem;

  class AutoReg {
    public:
      AutoReg(Pvl &pvl);

      virtual ~AutoReg();

      /**
       * Enumeration of the Register() method's return status.  All of the
       * enumerations describe a failure to register except "Success".  These status
       * values can be used to provide the user with more specific feedback on why
       * registration did not succeed.
       */
      enum RegisterStatus {
        Success, //!< Success
        PatternChipNotEnoughValidData,       //!< Not enough valid data in pattern chip
        FitChipNoData,                       //!< Fit chip did not have any valid data
        FitChipToleranceNotMet,              //!< Goodness of fit tolerance not satisfied
        SurfaceModelNotEnoughValidData,      //!< Not enough points to fit a surface model for sub-pixel accuracy
        SurfaceModelSolutionInvalid,         //!< Could not model surface for sub-pixel accuracy
        SurfaceModelDistanceInvalid,         //!< Surface model moves registration more than one pixel
        PatternZScoreNotMet,                 //!< Pattern data max or min does not pass the z-score test
        SurfaceModelEccentricityRatioNotMet, //!< Ellipse eccentricity of the surface model not satisfied
        SurfaceModelResidualToleranceNotMet, //!< Average residual of the surface model not satisfied
        AdaptiveAlgorithmFailed              //!< Error occured in Adaptive algorithm
      };

      //! Return pointer to pattern chip
      inline Chip *PatternChip() {
        return &p_patternChip;
      };

      //! Return pointer to search chip
      inline Chip *SearchChip() {
        return &p_searchChip;
      };

      //! Return pointer to fit chip
      inline Chip *FitChip() {
        return &p_fitChip;
      };

      //! Return pointer to reduced pattern chip
      inline Chip *ReducedPatternChip() {
        return &p_reducedPatternChip;
      };

      //! Return pointer to reduced search chip
      inline Chip *ReducedSearchChip() {
        return &p_reducedSearchChip;
      };

      //! Return pointer to reduced fix chip
      inline Chip *ReducedFitChip() {
        return &p_reducedFitChip;
      };

      void SetSubPixelAccuracy(bool on);
      void SetPatternValidPercent(const double percent);
      void SetSubsearchValidPercent(const double percent);
      void SetTolerance(double tolerance);
      void SetChipInterpolator(const string interpolator);
      void SetSurfaceModelWindowSize(int size);
      void SetSurfaceModelDistanceTolerance(double distance);
      void SetReductionFactor(int reductionFactor);
      void SetPatternZScoreMinimum(double minimum);
      void SetSurfaceModelEccentricityRatio(double ratioTolerance);
      void SetSurfaceModelResidualTolerance(double residualTolerance);

      /**
       * Determine if eccentricity tests should be performed during 
       * registration. 
       *  
       * If this method is not called, eccentricity testing defaults to test = 
       * false in the AutoReg object constructor. 
       * 
       * @param test Indicates whether to perform test
       */
      inline void SetEccentricityTesting(bool test) {
        p_testEccentricity = test;
      };

      /**
       * Determine  if residual tests should be performed during registration 
       *  
       * If this method is not called, residual testing defaults to test = 
       * false in the AutoReg object constructor. 
       *  
       * @param test Indicates whether to perform test
       */
      inline void SetResidualTesting(bool test) {
        p_testResidual = test;
      };

      //! Return pattern chip valid percent.  The default value is 
      double PatternValidPercent() const {
        return p_patternValidPercent;
      };

      //! Return subsearch chip valid percent
      double SubsearchValidPercent() const {
        return p_subsearchValidPercent;
      };

      //! Return match algorithm tolerance
      inline double Tolerance() const {
        return p_tolerance;
      };

      //! Return distance tolerance
      double DistanceTolerance() const {
        return p_distanceTolerance;
      };

      //! Return eccentricity tolerance represented as the A component in an A;1 ratio
      double EccentricityRatioTolerance() const {
        return p_surfaceModelEccentricityRatioTolerance;
      };

      //! Return residual tolerance
      double ResidualTolerance() const {
        return p_surfaceModelResidualTolerance;
      };

      /**
       * Return the distance point moved
       *
       * @param sampDistance Sample movement
       * @param lineDistance Line movement
       */
      void Distance(double &sampDistance, double &lineDistance) {
        sampDistance = p_sampMovement;
        lineDistance = p_lineMovement;
      }

      AutoReg::RegisterStatus Register();

      //! Return the goodness of fit of the match algorithm
      inline double GoodnessOfFit() const {
        return p_goodnessOfFit;
      };

      //! Return the eccentricity of the surface model, null if not calculated
      inline double Eccentricity() const {
        return p_surfaceModelEccentricity;
      };

      //! Return the eccentricity of the surface model as the A component of an of an A:1 ratio
      inline double EccentricityRatio() const {
        return p_surfaceModelEccentricityRatio;
      };

      //! Return the average residual of the surface model, null if not calculated
      inline double AverageResidual() const {
        return p_surfaceModelAvgResidual;
      };

      inline bool IsIdeal(double fit);

      //! Return the search chip sample that best matched
      inline double ChipSample() const {
        return p_chipSample;
      };

      //! Return the search chip line that best matched
      inline double ChipLine() const {
        return p_chipLine;
      };

      //! Return the search chip cube sample that best matched
      inline double CubeSample() const {
        return p_cubeSample;
      };

      //! Return the search chip cube line that best matched
      inline double CubeLine() const {
        return p_cubeLine;
      };

      //! Return minimumPatternZScore
      double MinimumZScore() const {
        return p_minimumPatternZScore;
      };

      /**
       * Return the ZScores of the pattern chip
       *
       * @param score1 First Z Score
       * @param score2 Second Z Score
       */
      void ZScores(double &score1, double &score2) const {
        score1 = p_ZScoreMin;
        score2 = p_ZScoreMax;
      }

      //! Return eccentricity tolerance
      double EccentricityTolerance() const {
        return p_surfaceModelEccentricityTolerance;
      };

      Pvl RegistrationStatistics();

      /**
       * Return if the algorithm is an adaptive pattern matching
       * technique
       */
      virtual bool IsAdaptive() {
        return false;
      }

      /**
       * Returns the name of the algorithm.
       *
       *
       * @return std::string
       */
      virtual string AlgorithmName() const = 0;

      PvlGroup RegTemplate();

    protected:
      /**
       * Sets the search chip subpixel sample that matches the pattern
       * tack sample.
       *
       * @param sample Value to set for search chip subpixel sample
       */
      inline void SetChipSample(double sample) {
        p_chipSample = sample;
      };

      /**
       * Sets the search chip subpixel line that matches the pattern
       * tack line.
       *
       *
       * @param line Value to set for search chip subpixel line
       */
      inline void SetChipLine(double line) {
        p_chipLine = line;
      };

      /**
       * Sets the goodness of fit for adaptive algorithms
       *
       * @param fit Fit value to set
       */
      inline void SetGoodnessOfFit(double fit) {
        p_bestFit = fit;
      };

      virtual AutoReg::RegisterStatus AdaptiveRegistration(Chip &sChip,
                                                           Chip &pChip,
                                                           Chip &fChip,
                                                           int startSamp,
                                                           int startLine,
                                                           int endSamp,
                                                           int endLine,
                                                           int bestSamp,
                                                           int bestLine);
      void Parse(Pvl &pvl);
      virtual bool CompareFits(double fit1, double fit2);
      bool ModelSurface(vector<double> &x, vector<double> &y,
                        vector<double> &z);
      Chip Reduce(Chip &chip, int reductionFactor);

      /**
       * Returns the ideal (perfect) fit that could be returned by the
       * MatchAlgorithm.
       *
       *
       * @return double
       */
      virtual double IdealFit() const = 0;

      /**
       *  Given two identically sized chips return a double that
       *  indicates how well they match.  For example, a correlation
       *  match algorithm would return a correlation coefficient
       *  ranging from -1 to 1.
       *
       *
       * @param pattern Pattern chip to match against
       * @param subsearch Subchip of the search chip to match with
       *
       * @return double
       */
      virtual double MatchAlgorithm(Chip &pattern, Chip &subsearch) = 0;

      PvlObject p_template; //!< AutoRegistration object that created this projection

      /**
       * @brief Provide (adaptive) algorithms a chance to report results
       *
       * Provide Adaptive objects the opportunity to report behavior.  It is
       * called at the final step prior to program termination.
       *
       * @param pvl Pvl structure to add report to
       *
       * @return Pvl Results
       */
      virtual Pvl AlgorithmStatistics(Pvl &pvl) {
        return (pvl);
      }

    private:
      /**
       * Empty copy constructor.
       * @param original AutoReg object
       */
      AutoReg(const AutoReg &original) {};
      void Match(Chip &sChip, 
                 Chip &pChip, 
                 Chip &fChip, 
                 int startSamp, 
                 int endSamp, 
                 int startLine, 
                 int endLine);
      bool ComputeChipZScore(Chip &chip);
      void Init();

      Chip p_patternChip;                               //!< Chip to be matched
      Chip p_searchChip;                                //!< Chip to be searched for best registration
      Chip p_fitChip;                                   //!< Results from MatchAlgorithm() method
      Chip p_reducedPatternChip;                        //!< Pattern Chip with reduction factor
      Chip p_reducedSearchChip;                         //!< Search Chip with reduction factor
      Chip p_reducedFitChip;                            //!< Fit Chip with reduction factor

      bool p_subpixelAccuracy;                          //!< Indicates whether sub-pixel accuracy is enabled. Default is true.
      bool p_testEccentricity;                          //!< Indicates whether eccentricity tests should be performed.
      bool p_testResidual;                              //!< Indicates whether residual tests should be performed.

      //TODO: remove these after control points are refactored.
      int p_Total;                                      //!< Registration Statistics Total keyword.
      int p_Success;                                    //!< Registration statistics Success keyword.
      int p_PatternChipNotEnoughValidData;              //!< Registration statistics PatternNotEnoughValidData keyword.
      int p_PatternZScoreNotMet;                        //!< Registration statistics PatternZScoreNotMet keyword.
      int p_FitChipNoData;                              //!< Registration statistics FitChipNoData keyword.
      int p_FitChipToleranceNotMet;                     //!< Registration statistics FitChipToleranceNotMet keyword.
      int p_SurfaceModelNotEnoughValidData;             //!< Registration statistics SurfaceModelNotEnoughValidData keyword.
      int p_SurfaceModelSolutionInvalid;                //!< Registration statistics SurfaceModelSolutionInvalid keyword.
      int p_SurfaceModelDistanceInvalid;                //!< Registration statistics SurfaceModelDistanceInvalid keyword.
      int p_SurfaceModelEccentricityRatioNotMet;        //!< Registration statistics SurfaceModelEccentricityRatioNotMet keyword.
      int p_SurfaceModelResidualToleranceNotMet;        //!< Registration statistics SurfaceModelResidualToleranceNotMet keyword.

      double p_ZScoreMin;                                 //!< First Z-Score of pattern chip
      double p_ZScoreMax;                                 //!< Second Z-Score of pattern chip

      double p_minimumPatternZScore;                    //!< Minimum pattern Z-Score
      double p_patternValidPercent;                     //!< Percentage of data in pattern chip that must be valid
      double p_subsearchValidPercent;                   //!< Percentage of data in subsearch chip that must be valid

      double p_chipSample;                              //!< Chip sample
      double p_chipLine;                                //!< Chip line
      double p_cubeSample;                              //!< Cube sample
      double p_cubeLine;                                //!< Cube line
      double p_goodnessOfFit;                           //!< Goodness of fit of the match algorithm
      double p_tolerance;                               //!< Tolerance for acceptable goodness of fit in match algorithm

      int p_windowSize;                                 //!< Surface model window size
      double p_distanceTolerance;                       //!< Maximum distance the surface model solution may be from the best whole pixel fit in the fit chip
      double p_surfaceModelResidualTolerance;           //!< Defaults to 0.1 in case enabled directly by programmer
      double p_surfaceModelAvgResidual;                 //!< Surface model average residual calculated by ModelSurface()

      double p_surfaceModelEccentricityTolerance;       //!< Surface model eccentricity tolerance. Defaults to 2:1 in case enabled directly by programmer
      double p_surfaceModelEccentricityRatioTolerance;  //!< Surface model eccentricity ratio tolerance. Defaults to 2:1 in case enabled directly by programmer
      double p_surfaceModelEccentricity;                //!< Surface model eccentricity.
      double p_surfaceModelEccentricityRatio;           //!< Surface model eccentricity ratio.
      double p_bestFit;                                 //!< Goodness of fit for adaptive algorithms.
      int p_bestSamp;                                   //!< Sample value of best fit.
      int p_bestLine;                                   //!< Line value of best fit.
      double p_sampMovement;                            //!< The number of samples the point moved.
      double p_lineMovement;                            //!< The number of lines the point moved.
      int p_reduceFactor;                               //!< Reduction factor.
      Isis::AutoReg::RegisterStatus p_status;           //!< Registration status to be returned by Register().

  };
};

#endif
