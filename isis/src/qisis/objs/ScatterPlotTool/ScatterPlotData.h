#ifndef ScatterPlotData_h
#define ScatterPlotData_h

#include <qwt_raster_data.h>

#include <QScopedPointer>
#include <QVector>

#include "Stretch.h"

template <typename A, typename B> class QPair;
template <typename T> class QVector;

class QwtDoubleRange;

namespace Isis {
  class Cube;

  /**
   * This is the QwtRasterData for a scatter plot. This gives Qwt values to
   *   put in each bin for a spectrogram, effectively making the scatter plot.
   *
   * @author ????-??-?? Unknown
   *
   * @internal
   */
  class ScatterPlotData : public QwtRasterData {
    public:
      ScatterPlotData(Cube *xCube, int xCubeBand, int xBinCount,
                      Cube *yCube, int yCubeBand, int yBinCount,
                      QwtDoubleRange sampleRange, QwtDoubleRange lineRange);
      ScatterPlotData(const ScatterPlotData &other);

      ~ScatterPlotData();
      virtual QwtRasterData *copy() const;
      virtual QwtDoubleInterval range() const;
      virtual double value(double x, double y) const;

      double xCubeMin() const;
      double xCubeMax() const;
      double yCubeMin() const;
      double yCubeMax() const;

      void swap(ScatterPlotData &other);

      QPair<double, double> binXY(int binIndex) const;
      int binCount(int binIndex) const;
      int numberOfBins() const;

      QVector<double> discreteXValues() const;

      void alarm(double x, double y) const;
      void clearAlarms() const;

      ScatterPlotData &operator=(const ScatterPlotData &other);

    private:
      int binCount(int xIndex, int yIndex) const;
      int binIndex(int xIndex, int yIndex) const;
      int binIndex(double x, double y) const;
      QPair<int, int> binXYIndices(int binIndex) const;
      QPair<int, int> binXYIndices(double x, double y) const;

      QScopedPointer<Stretch> m_xDnToBinStretch; //!< Stretch 1
      QScopedPointer<Stretch> m_yDnToBinStretch; //!< Stretch 2

      /**
       * The bin counts stored by 2D (x/y) index position. The first dimension
       *   is the y-index.
       */
      QScopedPointer< QVector< QVector<int> > > m_counts;

      //! The maximum value in m_counts, stored for efficiency
      int m_maxCount;

      /**
       * map from bin index to alarm state (true for alarmed), mutable for
       *   efficiency (not necessity). This efficiency gain is very significant
       *   and easy to test - the scatter plot data must be copied every time
       *   the alarming changes if this isn't mutable. Copy-on-write might be
       *   able to remove the need for this mutable.
       */
      mutable QScopedPointer< QMap<int, bool> > m_alarmedBins;

      //! The minimum DN value for the x cube
      double m_xCubeMin;
      //! The maximum DN value for the x cube
      double m_xCubeMax;
      //! The minimum DN value for the y cube
      double m_yCubeMin;
      //! The maximum DN value for the y cube
      double m_yCubeMax;
  };
};

#endif
