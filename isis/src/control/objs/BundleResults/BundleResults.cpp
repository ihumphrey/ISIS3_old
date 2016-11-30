#include "BundleResults.h"

#include <QDataStream>
#include <QDebug>
#include <QString>
#include <QtGlobal> // qMax()
#include <QUuid>
#include <QXmlStreamWriter>

#include <boost/lexical_cast.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>

#include <H5Cpp.h>
#include <hdf5_hl.h>
#include <hdf5.h>

#include "Camera.h"
#include "ControlMeasure.h"
#include "ControlPoint.h"
#include "CorrelationMatrix.h"
#include "Distance.h"
#include "FileName.h"
#include "IString.h"
#include "MaximumLikelihoodWFunctions.h"
#include "Project.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "SerialNumberList.h"
#include "StatCumProbDistDynCalc.h"
#include "Statistics.h"
#include "XmlStackedHandlerReader.h"

using namespace boost::numeric::ublas;

namespace Isis {

  /**
   * Constructs a BundleSettings object.
   * 
   * @param parent The Qt-relationship parent.
   */
  BundleResults::BundleResults(QObject *parent) : QObject(parent) {

    initialize();

    m_id = new QUuid(QUuid::createUuid());
    m_correlationMatrix = new CorrelationMatrix;
    m_cumPro = new StatCumProbDistDynCalc;
    m_cumProRes = new StatCumProbDistDynCalc;

    // residual prob distribution is calculated even if there is no maximum likelihood estimation.
    // so set up the solver to have a node at every percent of the distribution
    initializeResidualsProbabilityDistribution(101);

  }


  /**
   * Construct this BundleSettings object from XML.
   *
   * @param bundleSettingsFolder Where the settings XML for this bundle adjustment
   *                             resides - /work/.../projectRoot/images/import1
   * @param xmlReader An XML reader that's up to a <bundleSettings/> tag.
   * @param parent The Qt-relationship parent.
   */
  BundleResults::BundleResults(Project *project, XmlStackedHandlerReader *xmlReader,
                               QObject *parent) : QObject(parent) {   
                               // TODO: does xml stuff need project???

    initialize();

    xmlReader->pushContentHandler(new XmlHandler(this, project));
    xmlReader->setErrorHandler(new XmlHandler(this, project));

  }


  /**
   * Constructs this BundleSettings object from Hdf5.
   * 
   * @param locationObject The Hdf5 project file to be used.
   * @param locationName The name of the project file containing the BundleResults.
   * 
   * @see openH5Group
   */
  BundleResults::BundleResults(H5::CommonFG &locationObject, QString locationName) {
    openH5Group(locationObject, locationName);
  }


  /**
   * Copy constructor for BundleResults.  Creates this BundleSettings object as a copy
   * of another BundleResults object.
   * 
   * @param src The other BundleResults object to be copied.
   */
  BundleResults::BundleResults(const BundleResults &src)
      : m_id(new QUuid(src.m_id->toString())),
        m_correlationMatrix(new CorrelationMatrix(*src.m_correlationMatrix)),
        m_numberFixedPoints(src.m_numberFixedPoints),
        m_numberIgnoredPoints(src.m_numberIgnoredPoints),
        m_numberHeldImages(src.m_numberHeldImages),
        m_rmsXResiduals(src.m_rmsXResiduals),
        m_rmsYResiduals(src.m_rmsYResiduals),
        m_rmsXYResiduals(src.m_rmsXYResiduals),
        m_rejectionLimit(src.m_rejectionLimit),
        m_numberObservations(src.m_numberObservations),
        m_numberRejectedObservations(src.m_numberRejectedObservations),
        m_numberUnknownParameters(src.m_numberUnknownParameters),
        m_numberImageParameters(src.m_numberImageParameters),
        m_numberConstrainedImageParameters(src.m_numberConstrainedImageParameters),
        m_numberConstrainedPointParameters(src.m_numberConstrainedPointParameters),
        m_numberConstrainedTargetParameters(src.m_numberConstrainedTargetParameters),
        m_degreesOfFreedom(src.m_degreesOfFreedom),
        m_sigma0(src.m_sigma0),
        m_elapsedTime(src.m_elapsedTime),
        m_elapsedTimeErrorProp(src.m_elapsedTimeErrorProp),
        m_radiansToMeters(src.m_radiansToMeters),
        m_converged(src.m_converged),
        m_bundleControlPoints(src.m_bundleControlPoints),
        m_outNet(src.m_outNet),
        m_iterations(src.m_iterations),
        m_observations(src.m_observations),
        m_rmsImageSampleResiduals(src.m_rmsImageSampleResiduals),
        m_rmsImageLineResiduals(src.m_rmsImageLineResiduals),
        m_rmsImageResiduals(src.m_rmsImageResiduals),
        m_rmsImageXSigmas(src.m_rmsImageXSigmas),    
        m_rmsImageYSigmas(src.m_rmsImageYSigmas),    
        m_rmsImageZSigmas(src.m_rmsImageZSigmas),    
        m_rmsImageRASigmas(src.m_rmsImageRASigmas),   
        m_rmsImageDECSigmas(src.m_rmsImageDECSigmas),  
        m_rmsImageTWISTSigmas(src.m_rmsImageTWISTSigmas),
        m_minSigmaLatitudeDistance(src.m_minSigmaLatitudeDistance),
        m_maxSigmaLatitudeDistance(src.m_maxSigmaLatitudeDistance),
        m_minSigmaLongitudeDistance(src.m_minSigmaLongitudeDistance),
        m_maxSigmaLongitudeDistance(src.m_maxSigmaLongitudeDistance),
        m_minSigmaRadiusDistance(src.m_minSigmaRadiusDistance),
        m_maxSigmaRadiusDistance(src.m_maxSigmaRadiusDistance),
        m_minSigmaLatitudePointId(src.m_minSigmaLatitudePointId),
        m_maxSigmaLatitudePointId(src.m_maxSigmaLatitudePointId),
        m_minSigmaLongitudePointId(src.m_minSigmaLongitudePointId),
        m_maxSigmaLongitudePointId(src.m_maxSigmaLongitudePointId),
        m_minSigmaRadiusPointId(src.m_minSigmaRadiusPointId),
        m_maxSigmaRadiusPointId(src.m_maxSigmaRadiusPointId),
        m_rmsSigmaLatitudeStats(src.m_rmsSigmaLatitudeStats),
        m_rmsSigmaLongitudeStats(src.m_rmsSigmaLongitudeStats),
        m_rmsSigmaRadiusStats(src.m_rmsSigmaRadiusStats),
        m_maximumLikelihoodFunctions(src.m_maximumLikelihoodFunctions),
        m_maximumLikelihoodIndex(src.m_maximumLikelihoodIndex),
        m_cumPro(new StatCumProbDistDynCalc(*src.m_cumPro)),
        m_cumProRes(new StatCumProbDistDynCalc(*src.m_cumProRes)), 
        m_maximumLikelihoodMedianR2Residuals(src.m_maximumLikelihoodMedianR2Residuals) {

  }


  /**
   * Destroys this BundleResults object.
   */
  BundleResults::~BundleResults() {
    
    delete m_id;
    m_id = NULL;

    delete m_correlationMatrix;
    m_correlationMatrix = NULL;

    delete m_cumPro;
    m_cumPro = NULL;

    delete m_cumProRes;
    m_cumProRes = NULL;

  }


  /**
   * Assignment operator for BundleResults. Overwrites this BundleResults object with
   * another BundleResults object.
   * 
   * @param src The other BundleResults object to be copied from.
   */
  BundleResults &BundleResults::operator=(const BundleResults &src) {

    if (&src != this) {
      delete m_id;
      m_id = NULL;
      m_id = new QUuid(src.m_id->toString());

      delete m_correlationMatrix;
      m_correlationMatrix = NULL;
      m_correlationMatrix = new CorrelationMatrix(*src.m_correlationMatrix);

      m_numberFixedPoints = src.m_numberFixedPoints;
      m_numberIgnoredPoints = src.m_numberIgnoredPoints;
      m_numberHeldImages = src.m_numberHeldImages;
      m_rmsXResiduals = src.m_rmsXResiduals;
      m_rmsYResiduals = src.m_rmsYResiduals;
      m_rmsXYResiduals = src.m_rmsXYResiduals;
      m_rejectionLimit = src.m_rejectionLimit;
      m_numberObservations = src.m_numberObservations;
      m_numberRejectedObservations = src.m_numberRejectedObservations;
      m_numberUnknownParameters = src.m_numberUnknownParameters;
      m_numberImageParameters = src.m_numberImageParameters;
      m_numberConstrainedImageParameters = src.m_numberConstrainedImageParameters;
      m_numberConstrainedPointParameters = src.m_numberConstrainedPointParameters;
      m_numberConstrainedTargetParameters = src.m_numberConstrainedTargetParameters;
      m_degreesOfFreedom = src.m_degreesOfFreedom;
      m_sigma0 = src.m_sigma0;
      m_elapsedTime = src.m_elapsedTime;
      m_elapsedTimeErrorProp = src.m_elapsedTimeErrorProp;
      m_radiansToMeters = src.m_radiansToMeters;
      m_converged = src.m_converged;
      m_bundleControlPoints = src.m_bundleControlPoints;
      m_outNet = src.m_outNet;
      m_iterations = src.m_iterations;
      m_observations = src.m_observations;
      m_rmsImageSampleResiduals = src.m_rmsImageSampleResiduals;
      m_rmsImageLineResiduals = src.m_rmsImageLineResiduals;
      m_rmsImageResiduals = src.m_rmsImageResiduals;
      m_rmsImageXSigmas = src.m_rmsImageXSigmas;
      m_rmsImageYSigmas = src.m_rmsImageYSigmas;
      m_rmsImageZSigmas = src.m_rmsImageZSigmas;
      m_rmsImageRASigmas = src.m_rmsImageRASigmas;
      m_rmsImageDECSigmas = src.m_rmsImageDECSigmas;
      m_rmsImageTWISTSigmas = src.m_rmsImageTWISTSigmas;
      m_minSigmaLatitudeDistance = src.m_minSigmaLatitudeDistance;
      m_maxSigmaLatitudeDistance = src.m_maxSigmaLatitudeDistance;
      m_minSigmaLongitudeDistance = src.m_minSigmaLongitudeDistance;
      m_maxSigmaLongitudeDistance = src.m_maxSigmaLongitudeDistance;
      m_minSigmaRadiusDistance = src.m_minSigmaRadiusDistance;
      m_maxSigmaRadiusDistance = src.m_maxSigmaRadiusDistance;
      m_minSigmaLatitudePointId = src.m_minSigmaLatitudePointId;
      m_maxSigmaLatitudePointId = src.m_maxSigmaLatitudePointId;
      m_minSigmaLongitudePointId = src.m_minSigmaLongitudePointId;
      m_maxSigmaLongitudePointId = src.m_maxSigmaLongitudePointId;
      m_minSigmaRadiusPointId = src.m_minSigmaRadiusPointId;
      m_maxSigmaRadiusPointId = src.m_maxSigmaRadiusPointId;
      m_rmsSigmaLatitudeStats = src.m_rmsSigmaLatitudeStats;
      m_rmsSigmaLongitudeStats = src.m_rmsSigmaLongitudeStats;
      m_rmsSigmaRadiusStats = src.m_rmsSigmaRadiusStats;
      m_maximumLikelihoodFunctions = src.m_maximumLikelihoodFunctions;
      m_maximumLikelihoodIndex = src.m_maximumLikelihoodIndex;

      delete m_cumPro;
      m_cumPro = NULL;
      m_cumPro = new StatCumProbDistDynCalc(*src.m_cumPro);

      delete m_cumProRes;
      m_cumProRes = NULL;
      m_cumProRes = new StatCumProbDistDynCalc(*src.m_cumProRes);

      m_maximumLikelihoodMedianR2Residuals = src.m_maximumLikelihoodMedianR2Residuals;
    }
    return *this;
  }


  /**
   * Initializes the BundleResults to a default state where all numeric members are set to
   * 0 or another default value, all QString members are set to empty, all QVectors and
   * QLists are cleared, and all other members are set to NULL.
   */
  void BundleResults::initialize() {
    m_id = NULL;
    m_correlationMatrix = NULL;

    m_numberFixedPoints = 0; // set in BA constructor->init->fillPointIndexMap
    m_numberIgnoredPoints = 0; // set in BA constructor->init->fillPointIndexMap


    // set in BundleAdjust init()
    m_numberHeldImages = 0;

    // members set while computing bundle stats
    m_rmsImageSampleResiduals.clear();
    m_rmsImageLineResiduals.clear();
    m_rmsImageResiduals.clear();
    m_rmsImageXSigmas.clear();
    m_rmsImageYSigmas.clear();
    m_rmsImageZSigmas.clear();
    m_rmsImageRASigmas.clear();
    m_rmsImageDECSigmas.clear();
    m_rmsImageTWISTSigmas.clear();

    // initialize lat/lon/rad boundaries
    m_minSigmaLatitudeDistance.setMeters(1.0e+12);
    m_maxSigmaLatitudeDistance.setMeters(0.0);
    m_minSigmaLongitudeDistance.setMeters(1.0e+12);
    m_maxSigmaLongitudeDistance.setMeters(0.0);;
    m_minSigmaRadiusDistance.setMeters(1.0e+12);
    m_maxSigmaRadiusDistance.setMeters(0.0);
    m_minSigmaLatitudePointId = "";
    m_maxSigmaLatitudePointId = "";
    m_minSigmaLongitudePointId = "";
    m_maxSigmaLongitudePointId = "";
    m_minSigmaRadiusPointId = "";
    m_maxSigmaRadiusPointId = "";

    m_rmsSigmaLatitudeStats = 0.0;
    m_rmsSigmaLongitudeStats = 0.0;
    m_rmsSigmaRadiusStats = 0.0;


    // set by compute residuals
    m_rmsXResiduals = 0.0;
    m_rmsYResiduals = 0.0;
    m_rmsXYResiduals = 0.0;

    // set by compute rejection limit
    m_rejectionLimit = 0.0;
    
    // set by flag outliers    
    m_numberRejectedObservations = 0;

    // set by formNormalEquations_CHOLMOD, formNormalEquations_SPECIALK, or solve
    m_numberObservations = 0;
    m_numberImageParameters = 0; 

// ??? unused variable ???    m_numberHeldPoints = 0;

    // set by formNormalEquations_CHOLMOD, formNormalEquations_SPECIALK, or
    // setParameterWeights (i.e. solve)
    m_numberConstrainedPointParameters = 0;
    m_numberConstrainedImageParameters = 0;
    m_numberConstrainedTargetParameters = 0;

    // set by initialize, formNormalEquations_CHOLMOD, formNormalEquations_SPECIALK, or solve
    m_numberUnknownParameters = 0;

    // solve and solve cholesky
    m_degreesOfFreedom = -1;
    m_iterations = 0;
    m_sigma0 = 0.0;
    m_elapsedTime = 0.0;
    m_elapsedTimeErrorProp = 0.0;
    m_converged = false; // or initialze method

    m_cumPro = NULL;
    m_maximumLikelihoodIndex = 0;
    m_maximumLikelihoodMedianR2Residuals = 0.0;
    m_maximumLikelihoodFunctions.clear();
    m_cumProRes = NULL;
    
    m_radiansToMeters = 0;
    m_observations.clear();
    m_outNet.clear();

  }


  /**
   * Resizes all image sigma vectors.
   * 
   * @param numberImages The new size for the image sigma vectors.
   */
  void BundleResults::resizeSigmaStatisticsVectors(int numberImages) {
    m_rmsImageXSigmas.resize(numberImages);
    m_rmsImageYSigmas.resize(numberImages);
    m_rmsImageZSigmas.resize(numberImages);
    m_rmsImageRASigmas.resize(numberImages);
    m_rmsImageDECSigmas.resize(numberImages);
    m_rmsImageTWISTSigmas.resize(numberImages);
  }


#if 0
  void BundleResults::setRmsImageResidualLists(QVector<Statistics> rmsImageLineResiduals,
                                               QVector<Statistics> rmsImageSampleResiduals,
                                               QVector<Statistics> rmsImageResiduals) {
    // QList??? jigsaw apptest gives - ASSERT failure in QList<T>::operator[]: "index out of range",
    m_rmsImageLineResiduals = rmsImageLineResiduals.toList();
    m_rmsImageSampleResiduals = rmsImageSampleResiduals.toList();
    m_rmsImageResiduals = rmsImageResiduals.toList();
  }
#endif


  /**
   * Sets the root mean square image residual Statistics lists.
   * 
   * @param rmsImageLineResiduals The new image line residuals list.
   * @param rmsImageSampleResiduals The new image sample residuals list.
   * @param rmsImageResiduals The new image residuals list.
   */
  void BundleResults::setRmsImageResidualLists(QList<Statistics> rmsImageLineResiduals,
                                               QList<Statistics> rmsImageSampleResiduals,
                                               QList<Statistics> rmsImageResiduals) {
    m_rmsImageLineResiduals = rmsImageLineResiduals;
    m_rmsImageSampleResiduals = rmsImageSampleResiduals;
    m_rmsImageResiduals = rmsImageResiduals;
  }


  /**
   * Sets the min and max sigma latitude distances and point ids.
   * 
   * @param minLatDist The new minimum sigma latitude distance.
   * @param maxLatDist The new maximum sigma latitude distance.
   * @param minLatPointId The new minimum sigma latitude point id.
   * @param maxLatPointId The new maximum sigma latitude point id.
   */
  void BundleResults::setSigmaLatitudeRange(Distance minLatDist, Distance maxLatDist,
                                            QString minLatPointId, QString maxLatPointId) {
    m_minSigmaLatitudeDistance = minLatDist;
    m_maxSigmaLatitudeDistance = maxLatDist;
    m_minSigmaLatitudePointId  = minLatPointId;
    m_maxSigmaLatitudePointId  = maxLatPointId;
  }


  /**
   * Sets the min and max sigma longitude distances and point ids.
   * 
   * @param minLonDist The new minimum sigma longitude distance.
   * @param maxLonDist The new maximum sigma longitude distance.
   * @param minLonPointId The new minimum sigma longitude point id.
   * @param maxLonPointId The new maximum sigma longitude point id.
   */
  void BundleResults::setSigmaLongitudeRange(Distance minLonDist, Distance maxLonDist,
                                             QString minLonPointId, QString maxLonPointId) {
    m_minSigmaLongitudeDistance = minLonDist;
    m_maxSigmaLongitudeDistance = maxLonDist;
    m_minSigmaLongitudePointId  = minLonPointId;
    m_maxSigmaLongitudePointId  = maxLonPointId;
  }


  /**
   * Sets the min and max sigma radius distances and point ids.
   * 
   * @param minRadDist The new minimum sigma radius distance.
   * @param maxRadDist The new maximum sigma radius distance.
   * @param minRadPointId The new minimum sigma radius point id.
   * @param maxRadPointId The new maximum sigma radius point id.
   */
  void BundleResults::setSigmaRadiusRange(Distance minRadDist, Distance maxRadDist,
                                          QString minRadPointId, QString maxRadPointId) {
    m_minSigmaRadiusDistance = minRadDist;
    m_maxSigmaRadiusDistance = maxRadDist;
    m_minSigmaRadiusPointId  = minRadPointId;
    m_maxSigmaRadiusPointId  = maxRadPointId;
  }


  /**
   * Sets the root mean square values of the adjusted latitiude sigmas, adjusted longitude sigmas,
   * and adjusted radius sigmas.
   * 
   * @param rmsFromSigmaLatStats The new RMS value of the adjusted latitude sigmas.
   * @param rmsFromSigmaLonStats The new RMS value of the adjusted longitude sigmas.
   * @param rmsFromSigmaRadStats The new RMS value of the adjusted radius sigmas.
   */
  void BundleResults::setRmsFromSigmaStatistics(double rmsFromSigmaLatStats,
                                                double rmsFromSigmaLonStats,
                                                double rmsFromSigmaRadStats) {
    m_rmsSigmaLatitudeStats = rmsFromSigmaLatStats;
    m_rmsSigmaLongitudeStats = rmsFromSigmaLonStats;
    m_rmsSigmaRadiusStats = rmsFromSigmaRadStats;
  }


  /** 
  * This method steps up the maximum likelihood estimation solution.  Up to three successive
  * solutions models are available.
  * 
  * @param modelsWithQuantiles The maixmum likelihood models and their quantiles.  If empty,
  *                            then maximum likelihood estimation will not be used.
  */
  void BundleResults::maximumLikelihoodSetUp(
      QList< QPair< MaximumLikelihoodWFunctions::Model, double > > modelsWithQuantiles) {


    // reinitialize variables if this setup has already been called
    m_maximumLikelihoodIndex = 0;
    m_maximumLikelihoodMedianR2Residuals = 0.0;

    // residual prob distribution is calculated even if there is no maximum likelihood estimation.
    // set up the solver to have a node at every percent of the distribution
    m_cumProRes = NULL;
    m_cumProRes = new StatCumProbDistDynCalc;
    initializeResidualsProbabilityDistribution(101);

    // if numberMaximumLikelihoodModels > 0, then MaximumLikeliHood Estimation is being used.
    for (int i = 0; i < modelsWithQuantiles.size(); i++) {

      // if maximum likelihood is being used, the cum prob calculator is initialized.
      if (i == 0) {
        m_cumPro = NULL;
        m_cumPro = new StatCumProbDistDynCalc;
        // set up the solver to have a node at every percent of the distribution
        initializeProbabilityDistribution(101);
      }

      // set up the w functions for the maximum likelihood estimation
      m_maximumLikelihoodFunctions.append(
          qMakePair(MaximumLikelihoodWFunctions(modelsWithQuantiles[i].first), 
                    modelsWithQuantiles[i].second));

    }
    

    //maximum likelihood estimation tiered solutions requiring multiple convergeances are supported,
    // this index keeps track of which tier the solution is in
    m_maximumLikelihoodIndex = 0;
  }


  /**
   * Prints out information about which tier the solution is in and the status of the residuals.
   */
  void BundleResults::printMaximumLikelihoodTierInformation() {
    printf("Maximum Likelihood Tier: %d\n", m_maximumLikelihoodIndex);
    if (numberMaximumLikelihoodModels() > m_maximumLikelihoodIndex) {
      // if maximum likelihood estimation is being used
      // at the end of every iteration
      // reset the tweaking contant to the desired quantile of the |residual| distribution
      double quantile = m_maximumLikelihoodFunctions[m_maximumLikelihoodIndex].second;
      double tc = m_cumPro->value(quantile);
      m_maximumLikelihoodFunctions[m_maximumLikelihoodIndex].first.setTweakingConstant(tc);
      //  print meadians of residuals
      m_maximumLikelihoodMedianR2Residuals = m_cumPro->value(0.5);
      printf("Median of R^2 residuals:  %lf\n", m_maximumLikelihoodMedianR2Residuals);

      //restart the dynamic calculation of the cumulative probility distribution of |R^2 residuals|
      // so it will be up to date for the next iteration
      initializeProbabilityDistribution(101);
    }
  }


  /**
   * Initializes or resets the cumulative probability distribution of |R^2 residuals|.
   * 
   * @param nodes The number of quantiles in the cumulative probability distribution.
   */
  void BundleResults::initializeProbabilityDistribution(unsigned int nodes) {
    m_cumPro->setQuantiles(nodes);
  }


  /**
   * Initializes or resets the cumulative probability distribution of residuals used for reporting.
   * 
   * @param nodes The number of quantiles in the cumulative probability distribution.
   */
  void BundleResults::initializeResidualsProbabilityDistribution(unsigned int nodes) {
    m_cumProRes->setQuantiles(nodes);
  }


  /**
   * Adds an observation to the cumulative probability distribution
   * of |R^2 residuals|.
   * 
   * @param observationValue The value of the added observation.
   */
  void BundleResults::addProbabilityDistributionObservation(double observationValue) {
    m_cumPro->addObs(observationValue);
  }


  /**
   * Adds an observation to the cumulative probability distribution
   * of residuals used for reporting.
   * 
   * @param observationValue The value of the added observation.
   */
  void BundleResults::addResidualsProbabilityDistributionObservation(double observationValue) {
    m_cumProRes->addObs(observationValue);
  }


  /**
   * Increases the value that indicates which stage the maximum likelihood adjustment
   * is currently on.
   */
  void BundleResults::incrementMaximumLikelihoodModelIndex() {
    m_maximumLikelihoodIndex++;
  }


  /**
   * Increase the number of 'fixed' (ground) points.
   */
  void BundleResults::incrementFixedPoints() {
    m_numberFixedPoints++;
  }


  /**
   * Returns the number of 'fixed' (ground) points.
   * 
   * @return @b int The number of fixed points.
   */
  int BundleResults::numberFixedPoints() const {
    return m_numberFixedPoints;
  }


  /**
   * Increases the number of 'held' images.
   */
  void BundleResults::incrementHeldImages() {
    m_numberHeldImages++;
  }


  /**
   * Returns the number of 'held' images.
   * 
   * @return @b int The number of held images.
   */
  int BundleResults::numberHeldImages() const {
    return m_numberHeldImages;
  }


  /**
   * Increase the number of ignored points.
   */
  void BundleResults::incrementIgnoredPoints() {
    m_numberIgnoredPoints++;
  }


  /**
   * Returns the number of ignored points.
   * 
   * @return @b int The number of ignored points.
   */
  int BundleResults::numberIgnoredPoints() const {
    return m_numberIgnoredPoints;
  }


  /**
   * Sets the root mean square of the x and y residuals.
   * 
   * @param rx The RMS value of the x residuals.
   * @param ry The RMS value of the y residuals.
   * @param rxy The RMS value of both the x and y residuals.
   */
  void BundleResults::setRmsXYResiduals(double rx, double ry, double rxy) {
    m_rmsXResiduals = rx;
    m_rmsYResiduals = ry;
    m_rmsXYResiduals = rxy;
  }


  /**
   * Sets the rejection limit.
   * 
   * @param rejectionLimit The rejection limit.
   */
  void BundleResults::setRejectionLimit(double rejectionLimit) {
    m_rejectionLimit = rejectionLimit;
  }


  /**
   * Sets the number of rejected observations.
   * 
   * @param numberRejectedObservations The number of rejected observations.
   */
  void BundleResults::setNumberRejectedObservations(int numberRejectedObservations) {
    m_numberRejectedObservations = numberRejectedObservations;
  }


  /**
   * Sets the number of observations.
   * 
   * @param numberObservations The number of observations.
   */
  void BundleResults::setNumberObservations(int numberObservations) {
    m_numberObservations = numberObservations;
  }


  /**
   * Sets the number of image parameters.
   * 
   * @param numberParameters The number of image parameters.
   */
  void BundleResults::setNumberImageParameters(int numberParameters) {
    m_numberImageParameters = numberParameters;
  }


  /**
   * Resets the number of contrained point parameters to 0.
   */
  void BundleResults::resetNumberConstrainedPointParameters() {
    m_numberConstrainedPointParameters = 0;
  }


  /**
   * Increase the number of contrained point parameters.
   * 
   * @param incrementAmount The amount to increase by.
   */
  void BundleResults::incrementNumberConstrainedPointParameters(int incrementAmount) {
    m_numberConstrainedPointParameters += incrementAmount;
  }


  /**
   * Resets the number of constrained image parameters to 0.
   */
  void BundleResults::resetNumberConstrainedImageParameters() {
    m_numberConstrainedImageParameters = 0;
  }


  /**
   * Increase the number of constrained image parameters.
   * 
   * @param incrementAmount The amount to increase by.
   */
  void BundleResults::incrementNumberConstrainedImageParameters(int incrementAmount) {
    m_numberConstrainedImageParameters += incrementAmount;
  }


  /**
   * Resets the number of constrained target parameters to 0.
   */
  void BundleResults::resetNumberConstrainedTargetParameters() {
    m_numberConstrainedTargetParameters = 0;
  }


  /**
   * Increases the number of constrained target parameters.
   * 
   * @param incrementAmount The amount to increase by.
   */
  void BundleResults::incrementNumberConstrainedTargetParameters(int incrementAmount) {
    m_numberConstrainedTargetParameters += incrementAmount;
  }


  /**
   * Sets the total number of parameters to solve for.
   * 
   * @param numberParameters The number of parameters to solve for.
   */
  void BundleResults::setNumberUnknownParameters(int numberParameters) {
    m_numberUnknownParameters = numberParameters;
  }


  /**
   * Computes the degrees of freedom of the bundle adjustment and stores it internally.
   */
  void BundleResults::computeDegreesOfFreedom() {
    m_degreesOfFreedom = m_numberObservations
                         + m_numberConstrainedPointParameters
                         + m_numberConstrainedImageParameters
                         + m_numberConstrainedTargetParameters
                         - m_numberUnknownParameters;
  }


  /**
   * Computes the sigma0 and stores it internally.
   * 
   * @param dvtpv The weighted sum of the squares of the residuals.  Computed by
   *              V transpose * P * V, where
   *              V is the vector of residuals and
   *              P is the weight matrix.
   * @param criteria The convergence criteria for the bundle adjustment.
   * 
   * @throws IException::Io "Computed degrees of freedom is invalid."
   */
  void BundleResults::computeSigma0(double dvtpv, BundleSettings::ConvergenceCriteria criteria) {
    computeDegreesOfFreedom();

    if (m_degreesOfFreedom > 0) {
      m_sigma0 = dvtpv / m_degreesOfFreedom;
    }
    else if (m_degreesOfFreedom == 0 && criteria == BundleSettings::ParameterCorrections) {
      m_sigma0 = dvtpv;
    }
    else {
      QString msg = "Computed degrees of freedom [" + toString(m_degreesOfFreedom)
                    + "] is invalid.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    m_sigma0 = sqrt(m_sigma0);
  }


  /**
   * Sets the degrees of freedom.
   * 
   * @param degreesOfFreedom The degrees of freedom.
   */
  void BundleResults::setDegreesOfFreedom(double degreesOfFreedom) { // old sparse
    m_degreesOfFreedom = degreesOfFreedom;
  }


  /**
   * Sets the sigma0.
   * 
   * @param sigma0 The sigma0.
   */
  void BundleResults::setSigma0(double sigma0) { // old sparse
    m_sigma0 = sigma0;
  }


  /**
   * Sets the elapsed time for the bundle adjustment.
   * 
   * @param time The elapsed time.
   */
  void BundleResults::setElapsedTime(double time) {
    m_elapsedTime = time;
  }


  /**
   * Sets the elapsed time for error propegation.
   * 
   * @param time The elapsed time.
   */
  void BundleResults::setElapsedTimeErrorProp(double time) {
    m_elapsedTimeErrorProp = time;
  }


  /**
   * Sets the radians to meters conversion constant for the target body.
   * 
   * @param rtm The (double) conversion factor.
   */
  void BundleResults::setRadiansToMeters(double rtm) {
    m_radiansToMeters = rtm;
  }


  /**
   * Sets if the bundle adjustment converged.
   * 
   * @param converged If the bundle adjustment converged.
   */
  void BundleResults::setConverged(bool converged) {
    m_converged = converged;
  }


  /**
   * Sets the bundle control point vector.
   * 
   * @param controlPoints The vector of BundleControlPointQsps.
   */
  void BundleResults::setBundleControlPoints(QVector<BundleControlPointQsp> controlPoints) {
    m_bundleControlPoints = controlPoints;
  }


  /**
   * Sets the output ControlNet.
   * 
   * @param outNet A QSharedPointer to the output ControlNet.
   */
  void BundleResults::setOutputControlNet(ControlNetQsp outNet) {
    m_outNet = outNet;
  }


  /**
   * Sets the number of iterations taken by the BundleAdjust.
   * 
   * @param iterations The number of iterations.
   */
  void BundleResults::setIterations(int iterations) {
    m_iterations = iterations;
  }


  /**
   * Sets the vector of BundleObservations.
   * 
   * @param observations The vector of BundleObservations.
   */
  void BundleResults::setObservations(BundleObservationVector observations) {
    m_observations = observations;
  }



  //************************* Accessors **********************************************************//
  
  /**
   * Returns the list of RMS image sample residuals statistics.
   * 
   * @return @b QList<Statistics> The RMS image sample residual statistics.
   */
  QList<Statistics> BundleResults::rmsImageSampleResiduals() const {
    return m_rmsImageSampleResiduals;
  }


  /**
   * Returns the list of RMS image line residuals statistics.
   * 
   * @return @b QList<Statistics> The RMS image line residual statistics.
   */
  QList<Statistics> BundleResults::rmsImageLineResiduals() const {
    return m_rmsImageLineResiduals;
  }


  /**
   * Returns the list of RMS image residuals statistics.
   * 
   * @return @b QList<Statistics> The RMS image residual statistics.
   */
  QList<Statistics> BundleResults::rmsImageResiduals() const {
    return m_rmsImageResiduals;
  }


  /**
   * Returns the list of RMS image x sigma statistics.
   * 
   * @return @b QList<Statistics> The RMS image x sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageXSigmas() const {
    return m_rmsImageXSigmas;
  }


  /**
   * Returns the list of RMS image y sigma statistics.
   * 
   * @return @b QList<Statistics> The RMS image y sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageYSigmas() const {
    return m_rmsImageYSigmas;
  }


  /**
   * Returns the list of RMS image z sigma statistics.
   * 
   * @return @b QList<Statistics> The RMS image z sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageZSigmas() const {
    return m_rmsImageZSigmas;
  }


  /**
   * Returns the list of RMS image right ascension sigma statistics.
   * 
   * @return @b QList<Statistics> The RMS image right ascension sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageRASigmas() const {
    return m_rmsImageRASigmas;
  }


  /**
   * Returns the list of RMS image declination sigma statistics.
   * 
   * @return @b QList<Statistics> The RMS image declination sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageDECSigmas() const {
    return m_rmsImageDECSigmas;
  }


  /**
   * Returns the list of RMS image twist sigma statistics.
   * 
   * @return @b QList<Statistics> The RMS image twist sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageTWISTSigmas() const {
    return m_rmsImageTWISTSigmas;
  }


  /**
   * Returns the minimum sigma latitude distance.
   * 
   * @return @b Distance The minimum sigma latitude.
   */
  Distance BundleResults::minSigmaLatitudeDistance() const {
    return m_minSigmaLatitudeDistance;
  }


  /**
   * Returns the maximum sigma latitude distance.
   * 
   * @return @b Distance The maximum sigma latitude.
   */
  Distance BundleResults::maxSigmaLatitudeDistance() const {
    return m_maxSigmaLatitudeDistance;
  }


  /**
   * Returns the minimum sigma longitude distance.
   * 
   * @return @b Distance The minimum sigma longitude.
   */
  Distance BundleResults::minSigmaLongitudeDistance() const {
    return m_minSigmaLongitudeDistance;
  }


  /**
   * Returns the maximum sigma longitude distance.
   * 
   * @return @b Distance The maximum sigma longitude.
   */
  Distance BundleResults::maxSigmaLongitudeDistance() const {
    return m_maxSigmaLongitudeDistance;
  }


  /**
   * Returns the minimum sigma redius distance.
   * 
   * @return @b Distance The minimum sigma redius.
   */
  Distance BundleResults::minSigmaRadiusDistance() const {
    return m_minSigmaRadiusDistance;
  }


  /**
   * Returns the maximum sigma redius distance.
   * 
   * @return @b Distance The maximum sigma radius.
   */
  Distance BundleResults::maxSigmaRadiusDistance() const {
    return m_maxSigmaRadiusDistance;
  }


  /**
   * Returns the minimum sigma latitude point id.
   * 
   * @return @b @QString The minimum sigma latitude point id.
   */
  QString BundleResults::minSigmaLatitudePointId() const {
    return m_minSigmaLatitudePointId;
  }


  /**
   * Returns the maximum sigma latitude point id.
   * 
   * @return @b @QString The maximum sigma latitude point id.
   */
  QString BundleResults::maxSigmaLatitudePointId() const {
    return m_maxSigmaLatitudePointId;
  }


  /**
   * Returns the minimum sigma longitude point id.
   * 
   * @return @b @QString The minimum sigma longitude point id.
   */
  QString BundleResults::minSigmaLongitudePointId() const {
    return m_minSigmaLongitudePointId;
  }


  /**
   * Returns the maximum sigma longitude point id.
   * 
   * @return @b @QString The maximum sigma longitude point id.
   */
  QString BundleResults::maxSigmaLongitudePointId() const {
    return m_maxSigmaLongitudePointId;
  }


  /**
   * Returns the minimum sigma radius point id.
   * 
   * @return @b @QString The minimum sigma radius point id.
   */
  QString BundleResults::minSigmaRadiusPointId() const {
    return m_minSigmaRadiusPointId;
  }


  /**
   * Returns the maximum sigma radius point id.
   * 
   * @return @b @QString The maximum sigma radius point id.
   */
  QString BundleResults::maxSigmaRadiusPointId() const {
    return m_maxSigmaRadiusPointId;
  }


  /**
   * Returns the RMS of the adjusted latitude sigmas.
   * 
   * @return @b double The RMS of the adjusted latitude sigmas.
   */
  double BundleResults::sigmaLatitudeStatisticsRms() const {
    return m_rmsSigmaLatitudeStats;
  }


  /**
   * Returns the RMS of the adjusted longitude sigmas.
   * 
   * @return @b double The RMS of the adjusted longitude sigmas.
   */
  double BundleResults::sigmaLongitudeStatisticsRms() const {
    return m_rmsSigmaLongitudeStats;
  }


  /**
   * Returns the RMS of the adjusted raidus sigmas.
   * 
   * @return @b double The RMS of the adjusted radius sigmas.
   */
  double BundleResults::sigmaRadiusStatisticsRms() const {
    return m_rmsSigmaRadiusStats;
  }


  /**
   * Returns the RMS of the x residuals.
   * 
   * @return @b double The RMS of the x residuals.
   */
  double BundleResults::rmsRx() const {
    return m_rmsXResiduals;
  }


  /**
   * Returns the RMS of the y residuals.
   * 
   * @return @b double The RMS of the y residuals.
   */
  double BundleResults::rmsRy() const {
    return m_rmsYResiduals;
  }


  /**
   * Returns the RMS of the x and y residuals.
   * 
   * @return @b double The RMS of the x and y residuals.
   */
  double BundleResults::rmsRxy() const {
    return m_rmsXYResiduals;
  }


  /**
   * Returns the rejection limit.
   * 
   * @return @b double The rejection limit.
   */
  double BundleResults::rejectionLimit() const {
    return m_rejectionLimit;
  }


  /**
   * Returns the radians to meters conversion factor for the target body.
   * 
   * @return @b double The conversion factor.
   */
  double BundleResults::radiansToMeters() const {
    return m_radiansToMeters;
  }


  /**
   * Returns the number of observation that were rejected.
   * 
   * @return @b int The number of rejected observations.
   */
  int BundleResults::numberRejectedObservations() const {
    return m_numberRejectedObservations;
  }


  /**
   * Returns the number of observations.
   * 
   * @return @b int The number of observations.
   */
  int BundleResults::numberObservations() const {
    return m_numberObservations;
  }


  /**
   * Returns the total number of image parameters.
   * 
   * @return @b int The total number of image parameters.
   */
  int BundleResults::numberImageParameters() const {
    return m_numberImageParameters;
  }


  /**
   * Returns the number of constrained point parameters.
   * 
   * @return @b int The number of constrained point parameters.
   */
  int BundleResults::numberConstrainedPointParameters() const {
    return m_numberConstrainedPointParameters;
  }


  /**
   * Returns the number of constrained image parameters.
   * 
   * @return @b int The number of constrained image parameters.
   */
  int BundleResults::numberConstrainedImageParameters() const {
    return m_numberConstrainedImageParameters;
  }


  /**
   * Return the number of constrained target parameters.
   * 
   * @return @b int The number of constrained target parameters.
   */
  int BundleResults::numberConstrainedTargetParameters() const {
    return m_numberConstrainedTargetParameters;
  }


  /**
   * Returns the number of unknown parameters.
   * 
   * @return @b int The number of unknown parameters.
   */
  int BundleResults::numberUnknownParameters() const {
    return m_numberUnknownParameters;
  }


  /**
   * Returns the degrees of freedom.
   * 
   * @return @b int the degrees of freedom.
   */
  int BundleResults::degreesOfFreedom() const {
    return m_degreesOfFreedom;
  }


  /**
   * Returns the Sigma0 of the bundle adjustment.
   * 
   * @return @b double The Sigma0.
   */
  double BundleResults::sigma0() const {
    return m_sigma0;
  }


  /**
   * Returns the elapsed time for the bundle adjustment.
   * 
   * @return @b double The elapsed time for the bundle adjustment.
   */
  double BundleResults::elapsedTime() const {
    return m_elapsedTime;
  }


  /**
   * Returns the elapsed time for error propagation.
   * 
   * @return @b double The elapsed time for error propagation.
   */
  double BundleResults::elapsedTimeErrorProp() const {
    return m_elapsedTimeErrorProp;
  }


  /**
   * Returns whether or not the bundle adjustment converged.
   * 
   * @return @b bool If the bundle adjustment converged. 
   */
  bool BundleResults::converged() const {
    return m_converged;
  }


  /**
   * Returns a reference to the BundleControlPoint vector.
   * 
   * @return @b QVector<BundleControlPointQsp>& The BundleControlPoint vector.
   */
  QVector<BundleControlPointQsp> &BundleResults::bundleControlPoints() {
    return m_bundleControlPoints;
  }


  /**
   * Returns a shared pointer to the output control network.
   * 
   * @return @b ControlNetQsp A shared pointer to the output control network.
   * 
   * @throws IException::Programmer "Output Control Network has not been set."
   */
  ControlNetQsp BundleResults::outputControlNet() const {
    if (!m_outNet) {
      throw IException(IException::Programmer, 
                       "Output Control Network has not been set.",
                       _FILEINFO_);
    }
    return m_outNet;
  }


  /**
   * Returns the number of iterations taken by the BundleAdjust.
   * 
   * @return @b int The number of iterations.
   */
  int BundleResults::iterations() const {
    return m_iterations;
  }


  /**
   * Returns a reference to the observations used by the BundleAdjust.
   * 
   * @return @b BundleObservationVector& A reference to the observation vector.
   */
  const BundleObservationVector &BundleResults::observations() const {
    return m_observations;
  }


  /**
   * Returns how many maximum likelihood models were used in the bundle adjustment.
   * 
   * @return @b int The number fo maximum likelihood models.
   */
  int BundleResults::numberMaximumLikelihoodModels() const {
    return m_maximumLikelihoodFunctions.size();
  }


  /**
   * Returns which step the bundle adjustment is on.
   * 
   * @return @b int The maximum likelihood model that the bundle adjustment is currently using.
   */
  int BundleResults::maximumLikelihoodModelIndex() const {
    return m_maximumLikelihoodIndex;
  }


  /**
   * Returns the cumulative probability distribution of the |R^2 residuals|.
   * 
   * @return @b StatCumProbDistDynCalc The cumulative probability distribution of the
   *                                   |R^2 residuals|.
   */
  StatCumProbDistDynCalc BundleResults::cumulativeProbabilityDistribution() const {
    return *m_cumPro;
  }


  /**
   * Returns the cumulative probability distribution of the residuals used for reporting.
   * 
   * @return @b StatCumProbDistDynCalc the cumulative probability distribution of the residuals.
   */
  StatCumProbDistDynCalc BundleResults::residualsCumulativeProbabilityDistribution() const {
    return *m_cumProRes;
  }


  /**
   * Returns the median of the |R^2 residuals|.
   * 
   * @return @b double The median of the |R^2 residuals|.
   */
  double BundleResults::maximumLikelihoodMedianR2Residuals() const {
    return m_maximumLikelihoodMedianR2Residuals;
  }


  /**
   * Returns the maximum likelihood model at the given index.
   * 
   * @param modelIndex The index of the maximum likelihood model to be returned.
   * 
   * @return @b MaximumLikelihoodWFunctions The maximum likelihood model at the input index.
   */
  MaximumLikelihoodWFunctions BundleResults::maximumLikelihoodModelWFunc(int modelIndex) const {
    return m_maximumLikelihoodFunctions[modelIndex].first;
  }


  /**
   * Returns the quantile of the maximum likelihood model at the given index.
   * 
   * @param modelIndex The index of the maximum likelihood model whose quantile will be returned.
   * 
   * @return @b double The quantile of the desired maximum likelihood model.
   */
  double BundleResults::maximumLikelihoodModelQuantile(int modelIndex) const {
    return m_maximumLikelihoodFunctions[modelIndex].second;
  }


//  QList< QPair< MaximumLikelihoodWFunctions, double > >
//      BundleResults::maximumLikelihoodModels() const {
//    return m_maximumLikelihoodFunctions;
//  }


  /**
   * Saves the BundleResults object as a PvlObject.
   * 
   * @param name The name of the PvlObject to save to.
   * 
   * @return @b PvlObject A PvlObject containing the BundleResults object's information.
   */
  PvlObject BundleResults::pvlObject(QString name) const {

    PvlObject pvl(name);

    pvl += PvlKeyword("NumberFixedPoints", toString(numberFixedPoints()));
    pvl += PvlKeyword("NumberIgnoredPoints", toString(numberIgnoredPoints()));
    pvl += PvlKeyword("NumberHeldImages", toString(numberHeldImages()));
    pvl += PvlKeyword("RMSResidualX", toString(rmsRx()));
    pvl += PvlKeyword("RMSResidualY", toString(rmsRy()));
    pvl += PvlKeyword("RMSResidualXY", toString(rmsRxy()));
    pvl += PvlKeyword("RejectionLimit", toString(rejectionLimit()));
    pvl += PvlKeyword("RadiansToMeters", toString(radiansToMeters()));
    pvl += PvlKeyword("NumberRejectedObservations", toString(numberRejectedObservations()));
    pvl += PvlKeyword("NumberObservations", toString(numberObservations()));
    pvl += PvlKeyword("NumberImageParameters", toString(numberImageParameters()));
    pvl += PvlKeyword("NumberConstrainedPointParameters",
                      toString(numberConstrainedPointParameters()));
    pvl += PvlKeyword("NumberConstrainedImageParameters",
                      toString(numberConstrainedImageParameters()));
    pvl += PvlKeyword("NumberConstrainedTargetParameters",
                      toString(numberConstrainedTargetParameters()));
    pvl += PvlKeyword("NumberUnknownParameters", toString(numberUnknownParameters()));
    pvl += PvlKeyword("DegreesOfFreedom", toString(degreesOfFreedom()));
    pvl += PvlKeyword("Sigma0", toString(sigma0()));
    pvl += PvlKeyword("ElapsedTime", toString(elapsedTime()));
    pvl += PvlKeyword("ElapsedTimeErrorProp", toString(elapsedTimeErrorProp()));
    pvl += PvlKeyword("Iterations", toString(iterations()));
    pvl += PvlKeyword("Converged", toString(converged()));
#if 0
    // loop through these ??? what value to store???
    pvl += PvlKeyword("RmsImageSampleResidualsSize", toString(m_rmsImageSampleResiduals.size());
    pvl += PvlKeyword("RmsImageLineResidualsSize",   toString(m_rmsImageLineResiduals.size());
    pvl += PvlKeyword("RmsImageResidualsSize",       toString(m_rmsImageResiduals.size());
    pvl += PvlKeyword("RmsImageXSigmasSize",         toString(m_rmsImageXSigmas.size());
    pvl += PvlKeyword("RmsImageYSigmasSize",         toString(m_rmsImageYSigmas.size());
    pvl += PvlKeyword("RmsImageZSigmasSize",         toString(m_rmsImageZSigmas.size());
    pvl += PvlKeyword("RmsImageRASigmasSize",        toString(m_rmsImageRASigmas.size());
    pvl += PvlKeyword("RmsImageDECSigmasSize",       toString(m_rmsImageDECSigmas.size());
    pvl += PvlKeyword("RmsImageTWISTSigmasSize",     toString(m_rmsImageTWISTSigmas.size());
#endif 
    pvl += PvlKeyword("MinSigmaLatitude", toString(minSigmaLatitudeDistance().meters()));
    pvl += PvlKeyword("MinSigmaLatitudePointId", minSigmaLatitudePointId());
    pvl += PvlKeyword("MaxSigmaLatitude", toString(maxSigmaLatitudeDistance().meters()));
    pvl += PvlKeyword("MaxSigmaLatitudePointId", maxSigmaLatitudePointId());
    pvl += PvlKeyword("MinSigmaLongitude", toString(minSigmaLongitudeDistance().meters()));
    pvl += PvlKeyword("MinSigmaLongitudePointId", minSigmaLongitudePointId());
    pvl += PvlKeyword("MaxSigmaLongitude", toString(maxSigmaLongitudeDistance().meters()));
    pvl += PvlKeyword("MaxSigmaLongitudePointId", maxSigmaLongitudePointId());
    pvl += PvlKeyword("MinSigmaRadius", toString(minSigmaRadiusDistance().meters()));
    pvl += PvlKeyword("MinSigmaRadiusPointId", minSigmaRadiusPointId());
    pvl += PvlKeyword("MaxSigmaRadius", toString(maxSigmaRadiusDistance().meters()));
    pvl += PvlKeyword("MaxSigmaRadiusPointId", maxSigmaRadiusPointId());
    pvl += PvlKeyword("RmsSigmaLat", toString(sigmaLatitudeStatisticsRms()));
    pvl += PvlKeyword("RmsSigmaLon", toString(sigmaLongitudeStatisticsRms()));
    pvl += PvlKeyword("RmsSigmaRad", toString(sigmaRadiusStatisticsRms()));
    pvl += PvlKeyword("NumberMaximumLikelihoodModels", toString(numberMaximumLikelihoodModels()));
    if (numberMaximumLikelihoodModels() > 0) {

      PvlKeyword models("MaximumLikelihoodModels");
      PvlKeyword quantiles("MaximumLikelihoodQuantiles"); 
      
      for (int i = 0; i < m_maximumLikelihoodFunctions.size(); i++) {
        models.addValue(MaximumLikelihoodWFunctions::modelToString(
                            m_maximumLikelihoodFunctions[i].first.model()));
        quantiles.addValue(toString(m_maximumLikelihoodFunctions[i].second));
      }
      pvl += models;
      pvl += quantiles;
      pvl += PvlKeyword("MaximumLikelihoodMedianR2Residuals", 
                          toString(m_maximumLikelihoodMedianR2Residuals));
    }

    if (m_correlationMatrix) {
      pvl += correlationMatrix().pvlObject();
    }
    else {
      pvl += PvlKeyword("CorrelationMatrix", "None");
    }

    return pvl;
  }


  /**
   * Returns the Correlation Matrix.
   *
   * @return @b CorrelationMatrix The correlation matrix.
   * 
   * @throws IException::Unknown "Correlation matrix for this bundle is NULL."
   */
  CorrelationMatrix BundleResults::correlationMatrix() const {
    if (m_correlationMatrix) {
      return *m_correlationMatrix;
    }
    else {
      throw IException(IException::Unknown, 
                       "Correlation matrix for this bundle is NULL.",
                       _FILEINFO_);
    }
  }


  /**
   * Set the covariance file name for the matrix used to calculate the correlation matrix.
   *
   * @param name The name of the file used to store the covariance matrix.
   */
  void BundleResults::setCorrMatCovFileName(FileName name) {
    correlationMatrix();// throw error if null
    m_correlationMatrix->setCovarianceFileName(name);
  }


  /**
   * Set the images and their associated parameters of the correlation matrix.
   *
   * @param imgsAndParams The QMap with all the images and parameters used for this bundle.
   */
  void BundleResults::setCorrMatImgsAndParams(QMap<QString, QStringList> imgsAndParams) {
    correlationMatrix();// throw error if null
    m_correlationMatrix->setImagesAndParameters(imgsAndParams);
  }


  /**
   * Saves the BundleResults object to an XML file.
   * 
   * @param stream The QXMLStreamWriter that will be used to write out the XML file.
   * @param project The project that the BundleResults object belongs to.
   */
  void BundleResults::save(QXmlStreamWriter &stream, const Project *project) const {
    // TODO: does xml stuff need project???

    stream.writeStartElement("bundleResults");
    stream.writeTextElement("id", m_id->toString());
 
//    stream.writeTextElement("instrumentId", m_instrumentId);

    stream.writeStartElement("correlationMatrix");
    stream.writeAttribute("correlationFileName",
                          correlationMatrix().correlationFileName().expanded()); 
    stream.writeAttribute("covarianceFileName",
                          correlationMatrix().covarianceFileName().expanded()); 
    stream.writeStartElement("imagesAndParameters");
    QMapIterator<QString, QStringList> imgParamIt(*correlationMatrix().imagesAndParameters());
    while (imgParamIt.hasNext()) {
      imgParamIt.next();
      stream.writeStartElement("image"); 
      stream.writeAttribute("id", imgParamIt.key());
      QStringList parameters = imgParamIt.value();
      for (int i = 0; i < parameters.size(); i++) {
        stream.writeTextElement("parameter", parameters[i]);
      }
      stream.writeEndElement(); // end image
      
    }
    stream.writeEndElement(); // end images and parameters
    stream.writeEndElement(); // end correlationMatrix
    
    stream.writeStartElement("generalStatisticsValues");
    stream.writeTextElement("numberFixedPoints", toString(numberFixedPoints()));
    stream.writeTextElement("numberIgnoredPoints", toString(numberIgnoredPoints()));
    stream.writeTextElement("numberHeldImages", toString(numberHeldImages()));
    stream.writeTextElement("rejectionLimit", toString(rejectionLimit()));
    stream.writeTextElement("numberRejectedObservations", toString(numberRejectedObservations()));
    stream.writeTextElement("numberObservations", toString(numberObservations()));
    stream.writeTextElement("numberImageParameters", toString(numberImageParameters()));
    stream.writeTextElement("numberConstrainedPointParameters",
                            toString(numberConstrainedPointParameters()));
    stream.writeTextElement("numberConstrainedImageParameters",
                            toString(numberConstrainedImageParameters()));
    stream.writeTextElement("numberConstrainedTargetParameters",
                            toString(numberConstrainedTargetParameters()));
    stream.writeTextElement("numberUnknownParameters", toString(numberUnknownParameters()));
    stream.writeTextElement("degreesOfFreedom", toString(degreesOfFreedom()));
    stream.writeTextElement("sigma0", toString(sigma0()));
    stream.writeTextElement("converged", toString(converged()));
    stream.writeEndElement(); // end generalStatisticsValues

    stream.writeStartElement("rms");
    stream.writeStartElement("residuals");
    stream.writeAttribute("x", toString(rmsRx())); 
    stream.writeAttribute("y", toString(rmsRy())); 
    stream.writeAttribute("xy", toString(rmsRxy())); 
    stream.writeEndElement(); // end residuals element
    stream.writeStartElement("sigmas");
    stream.writeAttribute("lat", toString(sigmaLatitudeStatisticsRms())); 
    stream.writeAttribute("lon", toString(sigmaLongitudeStatisticsRms())); 
    stream.writeAttribute("rad", toString(sigmaRadiusStatisticsRms())); 
    stream.writeEndElement(); // end sigmas element

    stream.writeStartElement("imageResidualsLists");
    stream.writeStartElement("residualsList");
    stream.writeAttribute("listSize", toString(rmsImageResiduals().size())); 
    for (int i = 0; i < m_rmsImageResiduals.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageResiduals[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end residuals list
    stream.writeStartElement("sampleList");
    stream.writeAttribute("listSize", toString(rmsImageSampleResiduals().size())); 
    for (int i = 0; i < m_rmsImageSampleResiduals.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageSampleResiduals[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end sample residuals list

    stream.writeStartElement("lineList");
    stream.writeAttribute("listSize", toString(rmsImageLineResiduals().size())); 
    for (int i = 0; i < m_rmsImageLineResiduals.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageLineResiduals[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end line residuals list
    stream.writeEndElement(); // end image residuals lists

    stream.writeStartElement("imageSigmasLists");
    stream.writeStartElement("xSigmas");
    stream.writeAttribute("listSize", toString(rmsImageXSigmas().size())); 
    for (int i = 0; i < m_rmsImageXSigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageXSigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    
    stream.writeEndElement(); // end x sigma list

    stream.writeStartElement("ySigmas");
    stream.writeAttribute("listSize", toString(rmsImageYSigmas().size())); 
    for (int i = 0; i < m_rmsImageYSigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageYSigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end y sigma list

    stream.writeStartElement("zSigmas");
    stream.writeAttribute("listSize", toString(rmsImageZSigmas().size())); 
    for (int i = 0; i < m_rmsImageZSigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageZSigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end z sigma list

    stream.writeStartElement("raSigmas");
    stream.writeAttribute("listSize", toString(rmsImageRASigmas().size())); 
    for (int i = 0; i < m_rmsImageRASigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageRASigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end ra sigma list

    stream.writeStartElement("decSigmas");
    stream.writeAttribute("listSize", toString(rmsImageDECSigmas().size())); 
    for (int i = 0; i < m_rmsImageDECSigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageDECSigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end dec sigma list

    stream.writeStartElement("twistSigmas");
    stream.writeAttribute("listSize", toString(rmsImageTWISTSigmas().size())); 
    for (int i = 0; i < m_rmsImageTWISTSigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageTWISTSigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end twist sigma list
    stream.writeEndElement(); // end sigmas lists
    stream.writeEndElement(); // end rms

    stream.writeStartElement("elapsedTime");
    stream.writeAttribute("time", toString(elapsedTime())); 
    stream.writeAttribute("errorProp", toString(elapsedTimeErrorProp())); 
    stream.writeEndElement(); // end elapsed time

    stream.writeStartElement("minMaxSigmas");
    stream.writeStartElement("minLat");
    stream.writeAttribute("value", toString(minSigmaLatitudeDistance().meters())); 
    stream.writeAttribute("pointId", minSigmaLatitudePointId()); 
    stream.writeEndElement();
    stream.writeStartElement("maxLat");
    stream.writeAttribute("value", toString(maxSigmaLatitudeDistance().meters())); 
    stream.writeAttribute("pointId", maxSigmaLatitudePointId()); 
    stream.writeEndElement();
    stream.writeStartElement("minLon");
    stream.writeAttribute("value", toString(minSigmaLongitudeDistance().meters())); 
    stream.writeAttribute("pointId", minSigmaLongitudePointId()); 
    stream.writeEndElement();
    stream.writeStartElement("maxLon");
    stream.writeAttribute("value", toString(maxSigmaLongitudeDistance().meters())); 
    stream.writeAttribute("pointId", maxSigmaLongitudePointId()); 
    stream.writeEndElement();
    stream.writeStartElement("minRad");
    stream.writeAttribute("value", toString(minSigmaRadiusDistance().meters())); 
    stream.writeAttribute("pointId", minSigmaRadiusPointId()); 
    stream.writeEndElement();
    stream.writeStartElement("maxRad");
    stream.writeAttribute("value", toString(maxSigmaRadiusDistance().meters())); 
    stream.writeAttribute("pointId", maxSigmaRadiusPointId()); 
    stream.writeEndElement();
    stream.writeEndElement(); // end minMaxSigmas

    // call max likelihood setup from startElement to fill the rest of these values... 
    stream.writeStartElement("maximumLikelihoodEstimation");
    stream.writeAttribute("numberModels", toString(numberMaximumLikelihoodModels())); 
    stream.writeAttribute("maximumLikelihoodIndex", toString(maximumLikelihoodModelIndex())); 
    stream.writeAttribute("maximumLikelihoodMedianR2Residuals",
                          toString(maximumLikelihoodMedianR2Residuals()));

    stream.writeStartElement("cumulativeProbabilityCalculator");
    cumulativeProbabilityDistribution().save(stream, project);
    stream.writeEndElement(); // end cumulativeProbabilityCalculator

    stream.writeStartElement("residualsCumulativeProbabilityCalculator");
    residualsCumulativeProbabilityDistribution().save(stream, project);
    stream.writeEndElement(); // end residualsCumulativeProbabilityCalculator

    for (int i = 0; i < numberMaximumLikelihoodModels(); i++) {
      stream.writeStartElement("model");
      stream.writeAttribute("modelNumber", toString(i+1)); 
      stream.writeAttribute("modelSelection", 
        MaximumLikelihoodWFunctions::modelToString(m_maximumLikelihoodFunctions[i].first.model()));
      stream.writeAttribute("tweakingConstant",
                            toString(m_maximumLikelihoodFunctions[i].first.tweakingConstant())); 
      stream.writeAttribute("quantile", toString(m_maximumLikelihoodFunctions[i].second));
      stream.writeEndElement(); // end this model
    }
    stream.writeEndElement(); // end maximumLikelihoodEstimation
    stream.writeEndElement(); // end bundleResults
  }


  /**
   * Constructs an XmlHandler used to save a BundleResults object.
   * 
   * @param statistics The BundleResults that the XmlHandler will save.
   * @param project The project that the BundleResults object belongs to.
   */
  BundleResults::XmlHandler::XmlHandler(BundleResults *statistics, Project *project) {
    // TODO: does xml stuff need project???
    m_xmlHandlerBundleResults = NULL;
    m_xmlHandlerProject = NULL;

    m_xmlHandlerBundleResults = statistics;
    m_xmlHandlerProject = project;   // TODO: does xml stuff need project???
    m_xmlHandlerCharacters = "";

    m_xmlHandlerResidualsListSize = 0;
    m_xmlHandlerSampleResidualsListSize = 0;
    m_xmlHandlerLineResidualsListSize = 0;
    m_xmlHandlerXSigmasListSize = 0;
    m_xmlHandlerYSigmasListSize = 0;
    m_xmlHandlerZSigmasListSize = 0;
    m_xmlHandlerRASigmasListSize = 0;
    m_xmlHandlerDECSigmasListSize = 0;
    m_xmlHandlerTWISTSigmasListSize = 0;
    m_xmlHandlerStatisticsList.clear();

  }


  /**
   * Destroys an XmlHandler.
   */
  BundleResults::XmlHandler::~XmlHandler() {
    // do not delete this pointer... we don't own it, do we???
    // passed into StatCumProbDistDynCalc constructor as pointer
    // delete m_xmlHandlerProject;    // TODO: does xml stuff need project???
    m_xmlHandlerProject = NULL;
    
    // delete m_xmlHandlerBundleResults;
    // m_xmlHandlerBundleResults = NULL;
    
  }
  

  /**
   * Writes a starting XML element
   * 
   * @param namespaceURI ???
   * @param localName ???
   * @param qName The name of the element.
   * @param atts The attributes of the element.
   * 
   * @return @b bool If the XmlHandler should continue to be used, usually true.
   */
  bool BundleResults::XmlHandler::startElement(const QString &namespaceURI, 
                                               const QString &localName,
                                               const QString &qName,
                                               const QXmlAttributes &atts) {
    m_xmlHandlerCharacters = "";

    if (XmlStackedHandler::startElement(namespaceURI, localName, qName, atts)) {
        
      if (qName == "correlationMatrix") {

        m_xmlHandlerBundleResults->m_correlationMatrix = NULL;
        m_xmlHandlerBundleResults->m_correlationMatrix = new CorrelationMatrix();

        QString correlationFileName = atts.value("correlationFileName");
        if (!correlationFileName.isEmpty()) {
          FileName correlationFile(correlationFileName);
          m_xmlHandlerBundleResults->m_correlationMatrix->setCorrelationFileName(correlationFile);
        }

        QString covarianceFileName = atts.value("covarianceFileName");
        if (!covarianceFileName.isEmpty()) {
          FileName covarianceFile(covarianceFileName);
          m_xmlHandlerBundleResults->m_correlationMatrix->setCovarianceFileName(covarianceFile);
        }

      }
      else if (qName == "image") {
        QString correlationMatrixImageId = atts.value("id");
        if (!correlationMatrixImageId.isEmpty()) {
          m_xmlHandlerCorrelationImageId = correlationMatrixImageId;
        }
      }
      else if (qName == "residuals") {
        
        QString rx = atts.value("x");
        if (!rx.isEmpty()) {
          m_xmlHandlerBundleResults->m_rmsXResiduals = toDouble(rx);
        }

        QString ry = atts.value("y");
        if (!ry.isEmpty()) {
          m_xmlHandlerBundleResults->m_rmsYResiduals = toDouble(ry);
        }

        QString rxy = atts.value("xy");
        if (!rxy.isEmpty()) {
          m_xmlHandlerBundleResults->m_rmsXYResiduals = toDouble(rxy);
        }

      }
      else if (qName == "sigmas") {

        QString lat = atts.value("lat");
        if (!lat.isEmpty()) {
          m_xmlHandlerBundleResults->m_rmsSigmaLatitudeStats = toDouble(lat);
        }

        QString lon = atts.value("lon");
        if (!lon.isEmpty()) {
          m_xmlHandlerBundleResults->m_rmsSigmaLongitudeStats = toDouble(lon);
        }

        QString rad = atts.value("rad");
        if (!rad.isEmpty()) {
          m_xmlHandlerBundleResults->m_rmsSigmaRadiusStats = toDouble(rad);
        }

      }
      else if (qName == "residualsList") {

        QString listSizeStr = atts.value("listSize");
        if (!listSizeStr.isEmpty()) {
          m_xmlHandlerResidualsListSize = toInt(listSizeStr);
        }

      }
      else if (qName == "sampleList") {

        QString listSizeStr = atts.value("listSize");
        if (!listSizeStr.isEmpty()) {
          m_xmlHandlerSampleResidualsListSize = toInt(listSizeStr);
        }

      }
      else if (qName == "lineList") {

        QString listSizeStr = atts.value("listSize");
        if (!listSizeStr.isEmpty()) {
          m_xmlHandlerLineResidualsListSize = toInt(listSizeStr);
        }

      }
      else if (qName == "xSigmas") {

        QString listSizeStr = atts.value("listSize");
        if (!listSizeStr.isEmpty()) {
          m_xmlHandlerXSigmasListSize = toInt(listSizeStr);
        }

      }
      else if (qName == "ySigmas") {

        QString listSizeStr = atts.value("listSize");
        if (!listSizeStr.isEmpty()) {
          m_xmlHandlerYSigmasListSize = toInt(listSizeStr);
        }

      }
      else if (qName == "zSigmas") {

        QString listSizeStr = atts.value("listSize");
        if (!listSizeStr.isEmpty()) {
          m_xmlHandlerZSigmasListSize = toInt(listSizeStr);
        }

      }
      else if (qName == "raSigmas") {

        QString listSizeStr = atts.value("listSize");
        if (!listSizeStr.isEmpty()) {
          m_xmlHandlerRASigmasListSize = toInt(listSizeStr);
        }

      }
      else if (qName == "decSigmas") {

        QString listSizeStr = atts.value("listSize");
        if (!listSizeStr.isEmpty()) {
          m_xmlHandlerDECSigmasListSize = toInt(listSizeStr);
        }

      }
      else if (qName == "twistSigmas") {

        QString listSizeStr = atts.value("listSize");
        if (!listSizeStr.isEmpty()) {
          m_xmlHandlerTWISTSigmasListSize = toInt(listSizeStr);
        }

      }
      else if (qName == "statisticsItem") {
        // add statistics object to the xml handler's current statistics list.
        m_xmlHandlerStatisticsList.append(
            new Statistics(m_xmlHandlerProject, reader()));
      }
      else if (qName == "elapsedTime") {

        QString time = atts.value("time");
        if (!time.isEmpty()) {
          m_xmlHandlerBundleResults->m_elapsedTime = toDouble(time);
        }

        QString errorProp = atts.value("errorProp");
        if (!errorProp.isEmpty()) {
          m_xmlHandlerBundleResults->m_elapsedTimeErrorProp = toDouble(errorProp);
        }

      }
// ???      else if (qName == "minMaxSigmaDistances") {
// ???        QString units = atts.value("units");
// ???        if (!QString::compare(units, "meters", Qt::CaseInsensitive)) {
// ???          QString msg = "Unable to read BundleResults xml. Sigma distances must be "
// ???                        "provided in meters.";
// ???          throw IException(IException::Io, msg, _FILEINFO_);
// ???        }
// ???      }
      else if (qName == "minLat") {

        QString minLat = atts.value("value");
        if (!minLat.isEmpty()) {
          m_xmlHandlerBundleResults->m_minSigmaLatitudeDistance.setMeters(toDouble(minLat));
        }

        QString minLatPointId = atts.value("pointId");
        if (!minLatPointId.isEmpty()) {
          m_xmlHandlerBundleResults->m_minSigmaLatitudePointId = minLatPointId;
        }

      }
      else if (qName == "maxLat") {

        QString maxLat = atts.value("value");
        if (!maxLat.isEmpty()) {
          m_xmlHandlerBundleResults->m_maxSigmaLatitudeDistance.setMeters(toDouble(maxLat));
        }

        QString maxLatPointId = atts.value("pointId");
        if (!maxLatPointId.isEmpty()) {
          m_xmlHandlerBundleResults->m_maxSigmaLatitudePointId = maxLatPointId;
        }

      }
      else if (qName == "minLon") {

        QString minLon = atts.value("value");
        if (!minLon.isEmpty()) {
          m_xmlHandlerBundleResults->m_minSigmaLongitudeDistance.setMeters(toDouble(minLon));
        }

        QString minLonPointId = atts.value("pointId");
        if (!minLonPointId.isEmpty()) {
          m_xmlHandlerBundleResults->m_minSigmaLongitudePointId = minLonPointId;
        }

      }
      else if (qName == "maxLon") {

        QString maxLon = atts.value("value");
        if (!maxLon.isEmpty()) {
          m_xmlHandlerBundleResults->m_maxSigmaLongitudeDistance.setMeters(toDouble(maxLon));
        }

        QString maxLonPointId = atts.value("pointId");
        if (!maxLonPointId.isEmpty()) {
          m_xmlHandlerBundleResults->m_maxSigmaLongitudePointId = maxLonPointId;
        }

      }
      else if (qName == "minRad") {

        QString minRad = atts.value("value");
        if (!minRad.isEmpty()) {
          m_xmlHandlerBundleResults->m_minSigmaRadiusDistance.setMeters(toDouble(minRad));
        }

        QString minRadPointId = atts.value("pointId");
        if (!minRadPointId.isEmpty()) {
          m_xmlHandlerBundleResults->m_minSigmaRadiusPointId = minRadPointId;
        }

      }
      else if (qName == "maxRad") {

        QString maxRad = atts.value("value");
        if (!maxRad.isEmpty()) {
          m_xmlHandlerBundleResults->m_maxSigmaRadiusDistance.setMeters(toDouble(maxRad));
        }

        QString maxRadPointId = atts.value("pointId");
        if (!maxRadPointId.isEmpty()) {
          m_xmlHandlerBundleResults->m_maxSigmaRadiusPointId = maxRadPointId;
        }

      }
      else if (qName == "maximumLikelihoodEstimation") {

        QString maximumLikelihoodIndex = atts.value("maximumLikelihoodIndex");
        if (!maximumLikelihoodIndex.isEmpty()) {
          m_xmlHandlerBundleResults->m_maximumLikelihoodIndex = toInt(maximumLikelihoodIndex);
        }

        QString maximumLikelihoodMedianR2Residuals =
        atts.value("maximumLikelihoodMedianR2Residuals");
        if (!maximumLikelihoodMedianR2Residuals.isEmpty()) {
          m_xmlHandlerBundleResults->m_maximumLikelihoodMedianR2Residuals =
            toDouble(maximumLikelihoodMedianR2Residuals);
        }

      }
      else if (qName == "model") {
        QString model = atts.value("modelSelection");
        QString tweakingConstant = atts.value("tweakingConstant");
        QString quantile = atts.value("quantile");
        bool validModel = true;
        if (model.isEmpty())            validModel = false;
        if (tweakingConstant.isEmpty()) validModel = false;
        if (quantile.isEmpty())         validModel = false;
        if (validModel) {
          m_xmlHandlerBundleResults->m_maximumLikelihoodFunctions.append(
              qMakePair(MaximumLikelihoodWFunctions(
                            MaximumLikelihoodWFunctions::stringToModel(model),
                            toDouble(tweakingConstant)),
                        toDouble(quantile)));
        }
      }
      else if (qName == "cumulativeProbabilityCalculator") {
        m_xmlHandlerBundleResults->m_cumPro = NULL;
        m_xmlHandlerBundleResults->m_cumPro =
          new StatCumProbDistDynCalc(m_xmlHandlerProject, reader());
      }
      else if (qName == "residualsCumulativeProbabilityCalculator") {
        m_xmlHandlerBundleResults->m_cumProRes = NULL;
        m_xmlHandlerBundleResults->m_cumProRes = new StatCumProbDistDynCalc(m_xmlHandlerProject,
                                                                            reader());
      }
    }
    return true;
  }


  /**
   * Adds a QString to the XmlHandler's internal character data.
   * 
   * @param ch The data to be added.
   * 
   * @return @b bool If false, then the data was not successfully added.
   */
  bool BundleResults::XmlHandler::characters(const QString &ch) {
    m_xmlHandlerCharacters += ch;
    return XmlStackedHandler::characters(ch);
  }


  /**
   * Writes an ending XML element.
   * 
   * @param namespaceURI ???
   * @param localName ???
   * @param qName The name of the element.
   * 
   * @return @b bool If the XmlHandler should continue to be used.
   */
  bool BundleResults::XmlHandler::endElement(const QString &namespaceURI, const QString &localName,
                                             const QString &qName) {

    if (!m_xmlHandlerCharacters.isEmpty()) {
      if (qName == "id") {
        m_xmlHandlerBundleResults->m_id = NULL;
        m_xmlHandlerBundleResults->m_id = new QUuid(m_xmlHandlerCharacters);
      }
//      else if (qName == "instrumentId") {
//        m_xmlHandlerBundleResults->m_instrumentId = m_xmlHandlerCharacters;
//      }
      if (qName == "parameter") {
        // add the parameter to the current list
        m_xmlHandlerCorrelationParameterList.append(m_xmlHandlerCharacters);
      }
      if (qName == "image") {
        // add this image and its parameters to the map
        if (m_xmlHandlerCorrelationImageId != "") {
          m_xmlHandlerCorrelationMap.insert(m_xmlHandlerCorrelationImageId, 
                                            m_xmlHandlerCorrelationParameterList);
        }
        m_xmlHandlerCorrelationImageId = "";
        m_xmlHandlerCorrelationParameterList.clear();

      }
      if (qName == "imagesAndParameters") {
        // set the map after all images and parameters have been added
        if (!m_xmlHandlerCorrelationMap.isEmpty()) {
          m_xmlHandlerBundleResults->setCorrMatImgsAndParams(m_xmlHandlerCorrelationMap);
        }
      }
      else if (qName == "numberFixedPoints") {
        m_xmlHandlerBundleResults->m_numberFixedPoints = toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "numberIgnoredPoints") {
        m_xmlHandlerBundleResults->m_numberIgnoredPoints = toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "numberHeldImages") {
        m_xmlHandlerBundleResults->m_numberHeldImages = toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "rejectionLimit") {
        m_xmlHandlerBundleResults->m_rejectionLimit = toDouble(m_xmlHandlerCharacters);
      }
      else if (qName == "numberRejectedObservations") {
        m_xmlHandlerBundleResults->m_numberRejectedObservations = toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "numberObservations") {
        m_xmlHandlerBundleResults->m_numberObservations = toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "numberImageParameters") {
        m_xmlHandlerBundleResults->m_numberImageParameters = toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "numberConstrainedPointParameters") {
        m_xmlHandlerBundleResults->m_numberConstrainedPointParameters =
                                       toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "numberConstrainedImageParameters") {
        m_xmlHandlerBundleResults->m_numberConstrainedImageParameters =
                                       toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "numberConstrainedTargetParameters") {
        m_xmlHandlerBundleResults->m_numberConstrainedTargetParameters =
                                       toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "numberUnknownParameters") {
        m_xmlHandlerBundleResults->m_numberUnknownParameters = toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "degreesOfFreedom") {
        m_xmlHandlerBundleResults->m_degreesOfFreedom = toInt(m_xmlHandlerCharacters);
      }
      else if (qName == "sigma0") {
        m_xmlHandlerBundleResults->m_sigma0 = toDouble(m_xmlHandlerCharacters);
      }
      else if (qName == "converged") {
        m_xmlHandlerBundleResults->m_converged = toBool(m_xmlHandlerCharacters);
      }
      // copy the xml handler's statistics list to the appropriate bundle statistics list
      else if (qName == "residualsList") {
        // do this check or assume the xml is valid???
        // ??? if (m_xmlHandlerResidualsListSize != m_xmlHandlerStatisticsList.size()) {
        // ???   throw IException(IException::Unknown, 
        // ???                    "Unable to read xml file. Invalid residualsList", _FILEINFO_);
        // ??? }
        for (int i = 0; i < m_xmlHandlerStatisticsList.size(); i++) {
          m_xmlHandlerBundleResults->m_rmsImageResiduals.append(m_xmlHandlerStatisticsList[i]);
        }
        m_xmlHandlerStatisticsList.clear();
      }
      else if (qName == "sampleList") {
        // ??? if (m_xmlHandlerSampleResidualsListSize != m_xmlHandlerStatisticsList.size()) {
        // ???   throw IException(IException::Unknown, 
        // ???                    "Unable to read xml file. Invalid sampleList", _FILEINFO_);
        // ??? }
        for (int i = 0; i < m_xmlHandlerStatisticsList.size(); i++) {
          m_xmlHandlerBundleResults->m_rmsImageSampleResiduals.append(
                                                                   m_xmlHandlerStatisticsList[i]);
        }
        m_xmlHandlerStatisticsList.clear();
      }
      else if (qName == "lineList") {
        // ??? if (m_xmlHandlerLineResidualsListSize != m_xmlHandlerStatisticsList.size()) {
        // ???   throw IException(IException::Unknown, 
        // ???                    "Unable to read xml file. Invalid lineList", _FILEINFO_);
        // ??? }
        for (int i = 0; i < m_xmlHandlerStatisticsList.size(); i++) {
          m_xmlHandlerBundleResults->m_rmsImageLineResiduals.append(m_xmlHandlerStatisticsList[i]);
        }
        m_xmlHandlerStatisticsList.clear();
      }
      else if (qName == "xSigmas") {
        // ??? if (m_xmlHandlerXSigmasListSize != m_xmlHandlerStatisticsList.size()) {
        // ???   throw IException(IException::Unknown, 
        // ???                    "Unable to read xml file. Invalid xSigmas", _FILEINFO_); ???
        // }
        for (int i = 0; i < m_xmlHandlerStatisticsList.size(); i++) {
          m_xmlHandlerBundleResults->m_rmsImageXSigmas.append(m_xmlHandlerStatisticsList[i]);
        }
        m_xmlHandlerStatisticsList.clear();
      }
      else if (qName == "ySigmas") {
        // ??? if (m_xmlHandlerYSigmasListSize != m_xmlHandlerStatisticsList.size()) {
        // ???   throw IException(IException::Unknown, 
        // ???                    "Unable to read xml file. Invalid ySigmas", _FILEINFO_);
        // ??? }
        for (int i = 0; i < m_xmlHandlerStatisticsList.size(); i++) {
          m_xmlHandlerBundleResults->m_rmsImageYSigmas.append(m_xmlHandlerStatisticsList[i]);
        }
        m_xmlHandlerStatisticsList.clear();
      }
      else if (qName == "zSigmas") {
        // ??? if (m_xmlHandlerZSigmasListSize != m_xmlHandlerStatisticsList.size()) {
        // ???   throw IException(IException::Unknown, 
        // ???                    "Unable to read xml file. Invalid zSigmas", _FILEINFO_);
        // ??? }
        for (int i = 0; i < m_xmlHandlerStatisticsList.size(); i++) {
          m_xmlHandlerBundleResults->m_rmsImageZSigmas.append(m_xmlHandlerStatisticsList[i]);
        }
        m_xmlHandlerStatisticsList.clear();
      }
      else if (qName == "raSigmas") {
        // ??? if (m_xmlHandlerRASigmasListSize != m_xmlHandlerStatisticsList.size()) {
        // ???   throw IException(IException::Unknown, 
        // ???                    "Unable to read xml file. Invalid raSigmas", _FILEINFO_);
        // ??? }
        for (int i = 0; i < m_xmlHandlerStatisticsList.size(); i++) {
          m_xmlHandlerBundleResults->m_rmsImageRASigmas.append(m_xmlHandlerStatisticsList[i]);
        }
        m_xmlHandlerStatisticsList.clear();
      }
      else if (qName == "decSigmas") {
        // ??? if (m_xmlHandlerDECSigmasListSize != m_xmlHandlerStatisticsList.size()) {
        // ???   throw IException(IException::Unknown, 
        // ???                    "Unable to read xml file. Invalid decSigmas", _FILEINFO_);
        // ??? }
        for (int i = 0; i < m_xmlHandlerStatisticsList.size(); i++) {
          m_xmlHandlerBundleResults->m_rmsImageDECSigmas.append(m_xmlHandlerStatisticsList[i]);
        }
        m_xmlHandlerStatisticsList.clear();
      }
      else if (qName == "twistSigmas") {
        // ??? if (m_xmlHandlerTWISTSigmasListSize != m_xmlHandlerStatisticsList.size()) {
        // ???   throw IException(IException::Unknown, 
        // ???                    "Unable to read xml file. Invalid twistSigmas", _FILEINFO_);
        // ??? }
        for (int i = 0; i < m_xmlHandlerStatisticsList.size(); i++) {
          m_xmlHandlerBundleResults->m_rmsImageTWISTSigmas.append(m_xmlHandlerStatisticsList[i]);
        }
        m_xmlHandlerStatisticsList.clear();
      }
    }
    m_xmlHandlerCharacters = "";
    return XmlStackedHandler::endElement(namespaceURI, localName, qName);
  }


  /**
   * Writes the BundleResults object to a QDataStream.
   * 
   * @param stream The QDataStream that the BundleResults will be written to.
   * 
   * @return @b QDataStream& A reference to the QDataStream that
   *                         the BundleResults where written to.
   */
  QDataStream &BundleResults::write(QDataStream &stream) const {
    stream << m_id->toString()
           << *m_correlationMatrix
           << (qint32)m_numberFixedPoints
           << (qint32)m_numberIgnoredPoints
           << (qint32)m_numberHeldImages
           << m_rmsXResiduals << m_rmsYResiduals << m_rmsXYResiduals
           << m_rejectionLimit
           << (qint32)m_numberObservations
           << (qint32)m_numberRejectedObservations
           << (qint32)m_numberUnknownParameters
           << (qint32)m_numberImageParameters
           << (qint32)m_numberConstrainedImageParameters
           << (qint32)m_numberConstrainedPointParameters
           << (qint32)m_numberConstrainedTargetParameters
           << (qint32)m_degreesOfFreedom
           << m_sigma0
           << m_elapsedTime << m_elapsedTimeErrorProp
           << m_converged
           << m_rmsImageSampleResiduals << m_rmsImageLineResiduals
           << m_rmsImageResiduals
           << m_rmsImageXSigmas << m_rmsImageYSigmas << m_rmsImageZSigmas
           << m_rmsImageRASigmas << m_rmsImageDECSigmas << m_rmsImageTWISTSigmas
           << m_minSigmaLatitudeDistance.meters()
           << m_maxSigmaLatitudeDistance.meters()
           << m_minSigmaLongitudeDistance.meters()
           << m_maxSigmaLongitudeDistance.meters()
           << m_minSigmaRadiusDistance.meters()
           << m_maxSigmaRadiusDistance.meters()
           << m_minSigmaLatitudePointId        
           << m_maxSigmaLatitudePointId        
           << m_minSigmaLongitudePointId       
           << m_maxSigmaLongitudePointId       
           << m_minSigmaRadiusPointId          
           << m_maxSigmaRadiusPointId          
           << m_rmsSigmaLatitudeStats << m_rmsSigmaLongitudeStats << m_rmsSigmaRadiusStats
           << m_maximumLikelihoodFunctions
           << (qint32)m_maximumLikelihoodIndex << *m_cumPro << *m_cumProRes
           << m_maximumLikelihoodMedianR2Residuals;
    return stream;
  }


  /**
   * Reads the data from a QDataStream into the BundleResults object.
   * 
   * @param stream The QDataStream to read from.
   * 
   * @return @b QDataStream& A reference to the stream after reading from it.
   */
  QDataStream &BundleResults::read(QDataStream &stream) {
    QString id;
    CorrelationMatrix correlationMatrix;
    qint32 numberFixedPoints, numberIgnoredPoints, numberHeldImages, numberRejectedObservations,
           numberObservations, numberImageParameters, numberConstrainedPointParameters,
           numberConstrainedImageParameters, numberConstrainedTargetParameters,
           numberUnknownParameters, degreesOfFreedom, maximumLikelihoodIndex;
    double minSigmaLatitudeDistance, maxSigmaLatitudeDistance, minSigmaLongitudeDistance,
           maxSigmaLongitudeDistance, minSigmaRadiusDistance, maxSigmaRadiusDistance;   
    StatCumProbDistDynCalc cumPro;
    StatCumProbDistDynCalc cumProRes;

    stream >> id;
    stream >> correlationMatrix;
    stream >> numberFixedPoints;
    stream >> numberIgnoredPoints;
    stream >> numberHeldImages;
    stream >> m_rmsXResiduals >> m_rmsYResiduals >> m_rmsXYResiduals;
    stream >> m_rejectionLimit;
    stream >> numberObservations;
    stream >> numberRejectedObservations;
    stream >> numberUnknownParameters;
    stream >> numberImageParameters;
    stream >> numberConstrainedImageParameters;
    stream >> numberConstrainedPointParameters;
    stream >> numberConstrainedTargetParameters;
    stream >> degreesOfFreedom;
    stream >> m_sigma0;
    stream >> m_elapsedTime >> m_elapsedTimeErrorProp;
    stream >> m_converged;
    stream >> m_rmsImageSampleResiduals >> m_rmsImageLineResiduals;
    stream >> m_rmsImageResiduals;
    stream >> m_rmsImageXSigmas >> m_rmsImageYSigmas >> m_rmsImageZSigmas;
    stream >> m_rmsImageRASigmas >> m_rmsImageDECSigmas >> m_rmsImageTWISTSigmas;
    stream >> minSigmaLatitudeDistance;
    stream >> maxSigmaLatitudeDistance;
    stream >> minSigmaLongitudeDistance;
    stream >> maxSigmaLongitudeDistance;
    stream >> minSigmaRadiusDistance;
    stream >> maxSigmaRadiusDistance;
    stream >> m_minSigmaLatitudePointId;   
    stream >> m_maxSigmaLatitudePointId;   
    stream >> m_minSigmaLongitudePointId;  
    stream >> m_maxSigmaLongitudePointId;  
    stream >> m_minSigmaRadiusPointId;     
    stream >> m_maxSigmaRadiusPointId;     
    stream >> m_rmsSigmaLatitudeStats >> m_rmsSigmaLongitudeStats >> m_rmsSigmaRadiusStats;
    stream >> m_maximumLikelihoodFunctions;
    stream >> maximumLikelihoodIndex;
    stream >> cumPro >> cumProRes;
    stream >> m_maximumLikelihoodMedianR2Residuals;

    m_id = NULL;
    m_id = new QUuid(id);

    m_correlationMatrix = NULL;
    m_correlationMatrix = new CorrelationMatrix(correlationMatrix);

    m_numberFixedPoints                 = (int)numberFixedPoints;
    m_numberIgnoredPoints               = (int)numberIgnoredPoints;
    m_numberHeldImages                  = (int)numberHeldImages;
    m_numberRejectedObservations        = (int)numberRejectedObservations;
    m_numberObservations                = (int)numberObservations;
    m_numberImageParameters             = (int)numberImageParameters;
    m_numberConstrainedPointParameters  = (int)numberConstrainedPointParameters;
    m_numberConstrainedImageParameters  = (int)numberConstrainedImageParameters;
    m_numberConstrainedTargetParameters = (int)numberConstrainedTargetParameters;
    m_numberUnknownParameters           = (int)numberUnknownParameters;
    m_degreesOfFreedom                  = (int)degreesOfFreedom;
    m_maximumLikelihoodIndex            = (int)maximumLikelihoodIndex;

    m_minSigmaLatitudeDistance.setMeters(minSigmaLatitudeDistance); 
    m_maxSigmaLatitudeDistance.setMeters(maxSigmaLatitudeDistance); 
    m_minSigmaLongitudeDistance.setMeters(minSigmaLongitudeDistance);
    m_maxSigmaLongitudeDistance.setMeters(maxSigmaLongitudeDistance);
    m_minSigmaRadiusDistance.setMeters(minSigmaRadiusDistance);   
    m_maxSigmaRadiusDistance.setMeters(maxSigmaRadiusDistance);   

    m_cumPro = NULL;
    m_cumPro = new StatCumProbDistDynCalc(cumPro);

    m_cumProRes = NULL;
    m_cumProRes = new StatCumProbDistDynCalc(cumProRes);
    
    return stream;
  }


  /**
   * Insertion operator to save the BundleResults object to a QDataStream.
   * 
   * @param stream The QDataStream to write to.
   * @param bundleResults The BundleResults object to save.
   * 
   * @return @b QDataStream& A reference to the QDataStream after saving.
   *
   * @see write
   */
  QDataStream &operator<<(QDataStream &stream, const BundleResults &bundleResults) {
    return bundleResults.write(stream);
  }


  /**
   * Extraction operator to read the BundleResults object from a QDataStream.
   *
   * @param stream The QDataStream to read from.
   * @param bundleResults The BundleResults objec to read to.
   *
   * @return @b QDataStream& A reference to the QDataStream after reading.
   * 
   * @see read
   */
  QDataStream &operator>>(QDataStream &stream, BundleResults &bundleResults) {
    return bundleResults.read(stream);
  } 
  
  
  /**
   * Saves an hdf5 group
   * 
   * @param locationObject The hdf5 group.
   * @param localName The filename for the hdf5 group.
   * 
   * @throws IException::Unknown "H5 ATTRIBUTE exception handler has detected an error 
   *                              when invoking the function"
   * @throws IException::Unknown "Unable to save bundle results information to an HDF5 group."
   */  
  void BundleResults::createH5Group(H5::CommonFG &locationObject, QString locationName) const {
    try {
      // Try block to detect exceptions raised by any of the calls inside it
      try {
        /*
         * Turn off the auto-printing when failure occurs so that we can
         * handle the errors appropriately
         */
  //      H5::Exception::dontPrint();

        // create a results group to add to the given H5 object
        QString resultsGroupName = locationName + "/BundleResults"; 
        H5::Group resultsGroup = locationObject.createGroup(resultsGroupName.toLatin1());

        // use H5S_SCALAR data space type for single valued spaces
        H5::DataSpace spc(H5S_SCALAR);
        Attribute att;

        /* 
         * Add string attributes as predefined data type H5::PredType::C_S1 (string)
         */ 
        H5::StrType strDataType;
        int stringSize = 0; 

        //TODO: finish Correlation Matrix
        //Create a dataset with compression
  //    m_correlationMatrix->createH5Group(resultsGroup, resultsGroupName);
        QString correlationFileName = correlationMatrix().correlationFileName().expanded();
        stringSize = qMax(correlationFileName.length(), 1);
        strDataType = H5::StrType(H5::PredType::C_S1, stringSize); 
        att = resultsGroup.createAttribute("correlationFileName", strDataType, spc);
        att.write(strDataType, correlationFileName.toStdString());

        QString covarianceFileName = correlationMatrix().covarianceFileName().expanded();
        stringSize = qMax(covarianceFileName.length(), 1);
        strDataType = H5::StrType(H5::PredType::C_S1, stringSize); 
        att = resultsGroup.createAttribute("covarianceFileName", strDataType, spc);
        att.write(strDataType, covarianceFileName.toStdString());
        // TODO: table???
        // correlationMatrix().imagesAndParameters()???
        // QMapIterator<QString, QStringList> a list of images with their
        // corresponding parameters...


        /* 
         * Add integer attributes as predefined data type H5::PredType::NATIVE_INT
         */ 
        att = resultsGroup.createAttribute("numberFixedPoints", 
                                            H5::PredType::NATIVE_INT,
                                            spc);
        att.write(H5::PredType::NATIVE_INT, &m_numberFixedPoints);

        att = resultsGroup.createAttribute("numberIgnoredPoints", 
                                            H5::PredType::NATIVE_INT,
                                            spc);
        att.write(H5::PredType::NATIVE_INT, &m_numberIgnoredPoints);

        att = resultsGroup.createAttribute("numberHeldImages", 
                                            H5::PredType::NATIVE_INT,
                                            spc);
        att.write(H5::PredType::NATIVE_INT, &m_numberHeldImages);

        att = resultsGroup.createAttribute("numberObservations", 
                                            H5::PredType::NATIVE_INT,
                                            spc);
        att.write(H5::PredType::NATIVE_INT, &m_numberObservations);


        att = resultsGroup.createAttribute("numberRejectedObservations", 
                                            H5::PredType::NATIVE_INT,
                                            spc);
        att.write(H5::PredType::NATIVE_INT, &m_numberRejectedObservations);

        att = resultsGroup.createAttribute("numberImageParameters", 
                                            H5::PredType::NATIVE_INT,
                                            spc);
        att.write(H5::PredType::NATIVE_INT, &m_numberImageParameters);

        att = resultsGroup.createAttribute("numberConstrainedPointParameters", 
                                            H5::PredType::NATIVE_INT,
                                            spc);
        att.write(H5::PredType::NATIVE_INT, &m_numberConstrainedPointParameters);

        att = resultsGroup.createAttribute("numberConstrainedImageParameters", 
                                            H5::PredType::NATIVE_INT,
                                            spc);
        att.write(H5::PredType::NATIVE_INT, &m_numberConstrainedImageParameters);

        att = resultsGroup.createAttribute("numberUnknownParameters", 
                                            H5::PredType::NATIVE_INT,
                                            spc);
        att.write(H5::PredType::NATIVE_INT, &m_numberUnknownParameters);

        att = resultsGroup.createAttribute("degreesOfFreedom", 
                                            H5::PredType::NATIVE_INT,
                                            spc);
        att.write(H5::PredType::NATIVE_INT, &m_degreesOfFreedom);

        /* 
         * Add double attributes as predefined data type H5::PredType::NATIVE_DOUBLE
         */ 
        att = resultsGroup.createAttribute("rejectionLimit", 
                                            H5::PredType::NATIVE_DOUBLE,
                                            spc);
        att.write(H5::PredType::NATIVE_DOUBLE, &m_rejectionLimit);

        att = resultsGroup.createAttribute("sigma0", 
                                            H5::PredType::NATIVE_DOUBLE,
                                            spc);
        att.write(H5::PredType::NATIVE_DOUBLE, &m_sigma0);

        att = resultsGroup.createAttribute("elapsedTime", 
                                            H5::PredType::NATIVE_DOUBLE,
                                            spc);
        att.write(H5::PredType::NATIVE_DOUBLE, &m_elapsedTime);

        att = resultsGroup.createAttribute("elapsedTimeErrorProp", 
                                            H5::PredType::NATIVE_DOUBLE,
                                            spc);
        att.write(H5::PredType::NATIVE_DOUBLE, &m_elapsedTimeErrorProp);

        // todo: put rms in their own table/dataset/group???
        att = resultsGroup.createAttribute("rmsXResiduals", 
                                            H5::PredType::NATIVE_DOUBLE,
                                            spc);
        att.write(H5::PredType::NATIVE_DOUBLE, &m_rmsXResiduals);

        att = resultsGroup.createAttribute("rmsYResiduals", 
                                            H5::PredType::NATIVE_DOUBLE,
                                            spc);
        att.write(H5::PredType::NATIVE_DOUBLE, &m_rmsYResiduals);

        att = resultsGroup.createAttribute("rmsXYResiduals", 
                                            H5::PredType::NATIVE_DOUBLE,
                                            spc);
        att.write(H5::PredType::NATIVE_DOUBLE, &m_rmsXYResiduals);

        att = resultsGroup.createAttribute("rmsSigmaLatitudeStats", 
                                            H5::PredType::NATIVE_DOUBLE,
                                            spc);
        att.write(H5::PredType::NATIVE_DOUBLE, &m_rmsSigmaLatitudeStats);

        att = resultsGroup.createAttribute("rmsSigmaLongitudeStats", 
                                            H5::PredType::NATIVE_DOUBLE,
                                            spc);
        att.write(H5::PredType::NATIVE_DOUBLE, &m_rmsSigmaLongitudeStats);

        att = resultsGroup.createAttribute("rmsSigmaRadiusStats", 
                                            H5::PredType::NATIVE_DOUBLE,
                                            spc);
        att.write(H5::PredType::NATIVE_DOUBLE, &m_rmsSigmaRadiusStats);

        /*
         * Add bool attributes as predefined data type H5::PredType::NATIVE_HBOOL
         */ 
        att = resultsGroup.createAttribute("converged", 
                                            H5::PredType::NATIVE_HBOOL,
                                            spc);
        int converged = (int)m_converged;
        att.write(H5::PredType::NATIVE_HBOOL, &converged);
        /*
         * Add Statistics lists as data sets
         */ 
        QString dataSetName;
        H5::DataSet dataSet;
        hsize_t dims[1];
        H5::CompType compoundDataType = Statistics::compoundH5DataType();


        // IMAGE LINE RESIDUALS LIST
        {
          int listLength = 1;
          if (!m_rmsImageLineResiduals.isEmpty()) {
            listLength = m_rmsImageLineResiduals.size();
          }

          // Set the data space dimension to be the number of Statistics elements in this data set
          dims[0] = (hsize_t)listLength;
          H5::DataSpace dataSetSpace(1, dims);

          dataSetName = resultsGroupName + "/RmsImageLineResidualsStatistics"; 
          dataSet = resultsGroup.createDataSet(dataSetName.toLatin1(),
                                               compoundDataType,
                                               dataSetSpace);

          QByteArray byteArray;
          QDataStream stream(&byteArray, QIODevice::WriteOnly);
          stream.setByteOrder(QDataStream::LittleEndian);
          for (int i = 0; i < listLength; i++) {
            stream << m_rmsImageLineResiduals[i];
          }
          char *buf = byteArray.data();
          dataSet.write(buf, compoundDataType);
          dataSet.close();
        }

        // IMAGE SAMPLE RESIDUALS LIST
        {
          int listLength = 1;
          if (!m_rmsImageSampleResiduals.isEmpty()) {
            listLength = m_rmsImageSampleResiduals.size();
          }

          // Set the data space dimension to be the number of Statistics elements in this data set
          dims[0] = (hsize_t)listLength;
          H5::DataSpace dataSetSpace(1, dims);

          dataSetName = resultsGroupName + "/RmsImageSampleResidualsStatistics"; 
          dataSet = resultsGroup.createDataSet(dataSetName.toLatin1(),
                                               compoundDataType,
                                               dataSetSpace);

          QByteArray byteArray;
          QDataStream stream(&byteArray, QIODevice::WriteOnly);
          stream.setByteOrder(QDataStream::LittleEndian);
          for (int i = 0; i < listLength; i++) {
            stream << m_rmsImageSampleResiduals[i];
          }
          char *buf = byteArray.data();
          dataSet.write(buf, compoundDataType);
          dataSet.close();
        }

        // IMAGE RESIDUALS LIST
        {
          int listLength = 1;
          if (!m_rmsImageResiduals.isEmpty()) {
            listLength = m_rmsImageResiduals.size();
          }

          // Set the data space dimension to be the number of Statistics elements in this data set
          dims[0] = (hsize_t)listLength;
          H5::DataSpace dataSetSpace(1, dims);

          dataSetName = resultsGroupName + "/RmsImageResidualsStatistics"; 
          dataSet = resultsGroup.createDataSet(dataSetName.toLatin1(),
                                               compoundDataType,
                                               dataSetSpace);

          QByteArray byteArray;
          QDataStream stream(&byteArray, QIODevice::WriteOnly);
          stream.setByteOrder(QDataStream::LittleEndian);
          for (int i = 0; i < listLength; i++) {
            stream << m_rmsImageResiduals[i];
          }
          char *buf = byteArray.data();
          dataSet.write(buf, compoundDataType);
          dataSet.close();
        }

      }  // end of try block
      // catch failure caused by the Attribute operations
      catch ( H5::AttributeIException error ) {
        QString msg = "H5 Exception Message: " + QString::fromStdString(error.getDetailMsg());
        IException hpfError(IException::Unknown, msg, _FILEINFO_);
        msg = "H5 ATTRIBUTE exception handler has detected an error when invoking the function " 
              + QString::fromStdString(error.getFuncName()) + ".";
        throw IException(hpfError, IException::Unknown, msg, _FILEINFO_);
      }
      // catch failure caused by the DataSet operations
      catch ( H5::DataSetIException error ) {
        QString msg = "H5 Exception Message: " + QString::fromStdString(error.getDetailMsg());
        IException hpfError(IException::Unknown, msg, _FILEINFO_);
        msg = "H5 DATA SET exception handler has detected an error when invoking the function " 
              + QString::fromStdString(error.getFuncName()) + ".";
        throw IException(hpfError, IException::Unknown, msg, _FILEINFO_);
      }
      // catch failure caused by the DataSpace operations
      catch ( H5::DataSpaceIException error ) {
        QString msg = "H5 Exception Message: " + QString::fromStdString(error.getDetailMsg());
        IException hpfError(IException::Unknown, msg, _FILEINFO_);
        msg = "H5 DATA SPACE exception handler has detected an error when invoking the function " 
              + QString::fromStdString(error.getFuncName()) + ".";
        throw IException(hpfError, IException::Unknown, msg, _FILEINFO_);
      }
      // catch failure caused by the DataType operations
      catch ( H5::DataTypeIException error ) {
        QString msg = "H5 Exception Message: " + QString::fromStdString(error.getDetailMsg());
        IException hpfError(IException::Unknown, msg, _FILEINFO_);
        msg = "H5 DATA TYPE exception handler has detected an error when invoking the function " 
              + QString::fromStdString(error.getFuncName()) + ".";
        throw IException(hpfError, IException::Unknown, msg, _FILEINFO_);
      }
      // catch failure caused by the H5File operations
      catch ( H5::FileIException error ) {
        QString msg = "H5 Exception Message: " + QString::fromStdString(error.getDetailMsg());
        IException hpfError(IException::Unknown, msg, _FILEINFO_);
        msg = "H5 FILE exception handler has detected an error when invoking the function " 
              + QString::fromStdString(error.getFuncName()) + ".";
        throw IException(hpfError, IException::Unknown, msg, _FILEINFO_);
      }
      // catch failure caused by the Group operations
      catch ( H5::GroupIException error ) {
        QString msg = "H5 Exception Message: " + QString::fromStdString(error.getDetailMsg());
        IException hpfError(IException::Unknown, msg, _FILEINFO_);
        msg = "H5 GROUP exception handler has detected an error when invoking the function " 
              + QString::fromStdString(error.getFuncName()) + ".";
        throw IException(hpfError, IException::Unknown, msg, _FILEINFO_);
      }
      //??? how to improve printed msg using major/minor error codes?
      catch ( H5::Exception error ) {
        QString msg = "H5 Exception Message: " + QString::fromStdString(error.getDetailMsg());
        IException hpfError(IException::Unknown, msg, _FILEINFO_);
        msg = "H5 GENERAL exception handler has detected an error when invoking the function " 
              + QString::fromStdString(error.getFuncName()) + ".";
        throw IException(hpfError, IException::Unknown, msg, _FILEINFO_);
      }
    }
    catch (IException &e) {
      throw IException(e,
                       IException::Unknown, 
                       "Unable to save bundle results information to an HDF5 group.",
                       _FILEINFO_);
    }
      
  }

  
  /**
   * Reads from an hdf5 group.
   * 
   * @param locationObject The hdf5 group.
   * @param locationName The filename of the hdf5 group.
   * 
   * @throws IException::Unknown "H5 GENERAL exception handler has detected an error
   *                              when invoking the function"
   * @throws IException::Unknown "Unable to read bundle results information to an HDF5 group."
   */
  void BundleResults::openH5Group(H5::CommonFG &locationObject, QString locationName) {
    try {
      // Try block to detect exceptions raised by any of the calls inside it
      try {
        /*
         * Turn off the auto-printing when failure occurs so that we can
         * handle the errors appropriately
         */
  //      H5::Exception::dontPrint();

        // create a results group to add to the given H5 object
        QString resultsGroupName = locationName + "/BundleResults"; 
        H5::Group resultsGroup = locationObject.openGroup(resultsGroupName.toLatin1());

        Attribute att;

        //TODO: finish Correlation Matrix
        //Create a dataset with compression
  //    m_correlationMatrix->openH5Group(resultsGroup, resultsGroupName);

        /* 
         * read string atts as predefined data type H5::PredType::C_S1 (string)
         */ 
        H5::StrType strDataType;
        H5std_string strAttValue;

        att = resultsGroup.openAttribute("correlationFileName");
        strDataType = H5::StrType(H5::PredType::C_S1, att.getStorageSize());
        att.read(strDataType, strAttValue);
        m_correlationMatrix->setCorrelationFileName(QString::fromStdString(strAttValue));

        att = resultsGroup.openAttribute("covarianceFileName");
        strDataType = H5::StrType(H5::PredType::C_S1, att.getStorageSize());
        att.read(strDataType, strAttValue);
        m_correlationMatrix->setCovarianceFileName(QString::fromStdString(strAttValue));

        // TODO: table??? data set???
        // correlationMatrix().imagesAndParameters()???
        // QMapIterator<QString, QStringList> a list of images with their
        // corresponding parameters...


        /* 
         * read int attributes as predefined data type H5::PredType::NATIVE_INT
         */ 
        att = resultsGroup.openAttribute("numberFixedPoints");
        att.read(H5::PredType::NATIVE_INT, &m_numberFixedPoints);

        att = resultsGroup.openAttribute("numberIgnoredPoints");
        att.read(H5::PredType::NATIVE_INT, &m_numberIgnoredPoints);

        att = resultsGroup.openAttribute("numberHeldImages");
        att.read(H5::PredType::NATIVE_INT, &m_numberHeldImages);

        att = resultsGroup.openAttribute("numberObservations");
        att.read(H5::PredType::NATIVE_INT, &m_numberObservations);

        att = resultsGroup.openAttribute("numberRejectedObservations");
        att.read(H5::PredType::NATIVE_INT, &m_numberRejectedObservations);

        att = resultsGroup.openAttribute("numberImageParameters");
        att.read(H5::PredType::NATIVE_INT, &m_numberImageParameters);

        att = resultsGroup.openAttribute("numberConstrainedImageParameters");
        att.read(H5::PredType::NATIVE_INT, &m_numberConstrainedImageParameters);

        att = resultsGroup.openAttribute("numberConstrainedPointParameters");
        att.read(H5::PredType::NATIVE_INT, &m_numberConstrainedPointParameters);

        att = resultsGroup.openAttribute("numberUnknownParameters");
        att.read(H5::PredType::NATIVE_INT, &m_numberUnknownParameters);

        att = resultsGroup.openAttribute("degreesOfFreedom");
        att.read(H5::PredType::NATIVE_INT, &m_degreesOfFreedom);

        /* 
         * read double attributes as predefined data type H5::PredType::NATIVE_DOUBLE
         */ 
        att = resultsGroup.openAttribute("rejectionLimit");
        att.read(H5::PredType::NATIVE_DOUBLE, &m_rejectionLimit);

        att = resultsGroup.openAttribute("sigma0");
        att.read(H5::PredType::NATIVE_DOUBLE, &m_sigma0);

        att = resultsGroup.openAttribute("elapsedTime");
        att.read(H5::PredType::NATIVE_DOUBLE, &m_elapsedTime);

        att = resultsGroup.openAttribute("elapsedTimeErrorProp");
        att.read(H5::PredType::NATIVE_DOUBLE, &m_elapsedTimeErrorProp);

        // todo: put rms in their own table/dataset/group???
        att = resultsGroup.openAttribute("rmsXResiduals");
        att.read(H5::PredType::NATIVE_DOUBLE, &m_rmsXResiduals);

        att = resultsGroup.openAttribute("rmsYResiduals");
        att.read(H5::PredType::NATIVE_DOUBLE, &m_rmsYResiduals);

        att = resultsGroup.openAttribute("rmsXYResiduals");
        att.read(H5::PredType::NATIVE_DOUBLE, &m_rmsXYResiduals);

        att = resultsGroup.openAttribute("rmsSigmaLatitudeStats");
        att.read(H5::PredType::NATIVE_DOUBLE, &m_rmsSigmaLatitudeStats);

        att = resultsGroup.openAttribute("rmsSigmaLongitudeStats");
        att.read(H5::PredType::NATIVE_DOUBLE, &m_rmsSigmaLongitudeStats);

        att = resultsGroup.openAttribute("rmsSigmaRadiusStats");
        att.read(H5::PredType::NATIVE_DOUBLE, &m_rmsSigmaRadiusStats);

        /* 
         * read bool attributes as predefined data type H5::PredType::NATIVE_HBOOL
         */ 
        int boolAttValue = 0;
        att = resultsGroup.openAttribute("converged");
        att.read(H5::PredType::NATIVE_HBOOL, &boolAttValue);
        m_converged = (bool)boolAttValue;

        /*
         * read data sets of Statistics objects
         */ 
        QString dataSetName = "";
        H5::DataSet dataSet;
        H5::CompType compoundDataType = Statistics::compoundH5DataType();

        // IMAGE LINE RESIDUALS LIST
        {
          dataSetName = resultsGroupName + "/RmsImageLineResidualsStatistics";
          herr_t status = H5Gget_objinfo(resultsGroup.getId(), dataSetName.toLatin1(), 0, NULL);
          if (status != 0) {
            // group DNE...
            qDebug() << "didn't find or couldn't read stats list.";//???
          }
          try {

            // if this doesn't throw an error, then the group exists???
            H5G_stat_t info;
            resultsGroup.getObjinfo(dataSetName.toLatin1(), info);

            dataSet = resultsGroup.openDataSet(dataSetName.toLatin1());
            H5::DataSpace dataSetSpace = dataSet.getSpace();

            char statsList[dataSet.getStorageSize()];
            dataSet.read(statsList, compoundDataType);
    
            int listLength = dataSetSpace.getSimpleExtentNpoints();
            int statsSize = compoundDataType.getSize();
            for (int i = 0; i < listLength; i++) {
              QByteArray byteArray(&(statsList[i*statsSize]), statsSize);
              QDataStream stream(&byteArray, QIODevice::ReadOnly);
              stream.setByteOrder(QDataStream::LittleEndian);
      
              Statistics tempStats;
              stream >> tempStats;
              m_rmsImageLineResiduals.append(tempStats);
            }
    
          } 
          catch (H5::GroupIException groupError) {
            // don't do anything???
          }
        }
          
        // IMAGE SAMPLE RESIDUALS LIST
        {
          dataSetName = resultsGroupName + "/RmsImageSampleResidualsStatistics"; 
          herr_t status = H5Gget_objinfo(resultsGroup.getId(), dataSetName.toLatin1(), 0, NULL);
          if (status != 0) {
            // group DNE...
            qDebug() << "didn't find or couldn't read stats list.";
          }
          try {

            // if this doesn't throw an error, then the group exists???
            H5G_stat_t info;
            resultsGroup.getObjinfo(dataSetName.toLatin1(), info);

            dataSet = resultsGroup.openDataSet(dataSetName.toLatin1());
            H5::DataSpace dataSetSpace = dataSet.getSpace();

            char statsList[dataSet.getStorageSize()];
            dataSet.read(statsList, compoundDataType);
    
            int listLength = dataSetSpace.getSimpleExtentNpoints();
            int statsSize = compoundDataType.getSize();
            for (int i = 0; i < listLength; i++) {
              QByteArray byteArray(&(statsList[i*statsSize]), statsSize);
              QDataStream stream(&byteArray, QIODevice::ReadOnly);
              stream.setByteOrder(QDataStream::LittleEndian);
      
              Statistics tempStats;
              stream >> tempStats;
              m_rmsImageLineResiduals.append(tempStats);
            }
    
          } 
          catch (H5::GroupIException groupError) {
            // don't do anything???
          }
        }
        // IMAGE RESIDUALS LIST
        {
          dataSetName = resultsGroupName + "/RmsImageResidualsStatistics"; 
          herr_t status = H5Gget_objinfo(resultsGroup.getId(), dataSetName.toLatin1(), 0, NULL);
          if (status != 0) {
            // group DNE...
            qDebug() << "didn't find or couldn't read stats list.";//???
          }
          try {

            // if this doesn't throw an error, then the group exists???
            H5G_stat_t info;
            resultsGroup.getObjinfo(dataSetName.toLatin1(), info);

            dataSet = resultsGroup.openDataSet(dataSetName.toLatin1());
            H5::DataSpace dataSetSpace = dataSet.getSpace();

            char statsList[dataSet.getStorageSize()];
            dataSet.read(statsList, compoundDataType);
    
            int listLength = dataSetSpace.getSimpleExtentNpoints();
            int statsSize = compoundDataType.getSize();
            for (int i = 0; i < listLength; i++) {
              QByteArray byteArray(&(statsList[i*statsSize]), statsSize);
              QDataStream stream(&byteArray, QIODevice::ReadOnly);
              stream.setByteOrder(QDataStream::LittleEndian);
      
              Statistics tempStats;
              stream >> tempStats;
              m_rmsImageLineResiduals.append(tempStats);
            }
    
          } 
          catch (H5::GroupIException groupError) {
            // don't do anything???
          }
        }
      }
      catch (H5::Exception error) {  //??? how to improve printed msg using major/minor error codes?
        QString msg = "H5 Exception Message: " + QString::fromStdString(error.getDetailMsg());
        IException hpfError(IException::Unknown, msg, _FILEINFO_);
        msg = "H5 GENERAL exception handler has detected an error when invoking the function " 
              + QString::fromStdString(error.getFuncName()) + ".";
        throw IException(hpfError, IException::Unknown, msg, _FILEINFO_);
      }
    }
    catch (IException &e) {
      throw IException(e,
                       IException::Unknown, 
                       "Unable to read bundle results information to an HDF5 group.",
                       _FILEINFO_);
    }
  }
}
