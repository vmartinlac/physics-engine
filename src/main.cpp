#include <QSurfaceFormat>
#include <QApplication>
#include "World.h"
#include "MainWindow.h"
#include "PhysicalConstants.h"

int main(int num_args, char** args)
{
    QApplication app(num_args, args);

    World* world = new World;

    BodyPtr body1( new SphereBody(1.0, CTE_WOOD_DENSITY) );
    body1->position = osg::Vec3d(0.0, 0.0, 5.0);
    world->addBody(body1);

    BodyPtr body2( new BoxBody( osg::Vec3d(20.0, 20.0, 1.0), CTE_IRON_DENSITY ) );
    body2->position = osg::Vec3d(0.0, 0.0, -0.5);
    world->addBody(body2);

    world->syncRepresentation();

    MainWindow* w = new MainWindow;
    w->show();

    const int ret = app.exec();

    return ret;
}

