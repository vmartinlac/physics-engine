#include <iostream>

#include <osg/Geode>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <osgGA/TrackballManipulator>

#include <QEvent>
#include <QTimerEvent>
#include <QMouseEvent>

#include "ViewerWidget.h"
#include "World.h"

ViewerWidget::ViewerWidget( std::shared_ptr<World> world, QWidget* parent ) : QOpenGLWidget(parent), _world(world)
{
    osgGA::TrackballManipulator* manipulator = new osgGA::TrackballManipulator;

    _viewer = new osgViewer::Viewer;
    _viewer->setCameraManipulator(manipulator);
    _viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
    _viewer->setRunFrameScheme(osgViewer::Viewer::ON_DEMAND);
    _viewer->setSceneData(world->getRepresentation());

    _window = _viewer->setUpViewerAsEmbeddedInWindow(0, 0, width(), height());

    //setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(100, 100);
    _updateTimer = startTimer(30);
}

void ViewerWidget::timerEvent(QTimerEvent* ev)
{
    if(ev->timerId() == _updateTimer)
    {
        _world->syncRepresentation();
        update();
    }
}

void ViewerWidget::paintGL()
{
    _viewer->frame();
}

void ViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
    _window->getEventQueue()->mouseMotion(event->x(), event->y());
}

void ViewerWidget::initializeGL()
{
   ;
}

void ViewerWidget::mousePressEvent(QMouseEvent* event)
{
    unsigned int button = 0;

    switch (event->button()){
    case Qt::LeftButton:
        button = 1;
        break;
    case Qt::MiddleButton:
        button = 2;
        break;
    case Qt::RightButton:
        button = 3;
        break;
    default:
        break;
    }

    _window->getEventQueue()->mouseButtonPress(event->x(), event->y(), button);
}

void ViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    unsigned int button = 0;

    switch (event->button())
    {
    case Qt::LeftButton:
        button = 1;
        break;
    case Qt::MiddleButton:
        button = 2;
        break;
    case Qt::RightButton:
        button = 3;
        break;
    default:
        break;
    }

    _window->getEventQueue()->mouseButtonRelease(event->x(), event->y(), button);
}

void ViewerWidget::wheelEvent(QWheelEvent* event)
{
    const int delta = event->delta();
    osgGA::GUIEventAdapter::ScrollingMotion motion = delta > 0 ?  osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN;
    _window->getEventQueue()->mouseScroll(motion);
}

bool ViewerWidget::event(QEvent* event)
{
    bool handled = QOpenGLWidget::event(event);

    switch( event->type() )
    {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
        update();
        break;
    /*
    case QEvent::Timer:
        update();
        break;
    */
    default:
        break;
    };

    return handled;
}

void ViewerWidget::resizeGL(int width, int height)
{
    //_window->getEventQueue()->windowResize( this->x(), this->y(), width, height );
    _window->resized( this->x(), this->y(), width, height );

   /*
   std::vector<osg::Camera*> cameras;
   _viewer->getCameras( cameras );

   assert( cameras.size() == 1 );

   cameras.front()->setViewport( 0, 0, this->width(), this->height() );
   */
}

