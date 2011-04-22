#ifndef AbstractFilter_H
#define AbstractFilter_H

#include <QWidget>


class QButtonGroup;
class QHBoxLayout;


namespace Isis
{
  class ControlPoint;
  class ControlMeasure;
  class ControlCubeGraphNode;
  
  class AbstractFilter : public QWidget
  {
      Q_OBJECT

    public:
      AbstractFilter();
      virtual ~AbstractFilter();
      
      virtual bool evaluate(const ControlPoint *) const = 0;
      virtual bool evaluate(const ControlMeasure *) const = 0;
      virtual bool evaluate(const ControlCubeGraphNode *) const = 0;


    signals:
      void filterChanged();


    protected:
      virtual void nullify();
      virtual void createWidget();
      bool inclusive() const;


    protected:
      QHBoxLayout * mainLayout;


    private:
      QButtonGroup * buttonGroup;
  };
}

#endif
