#include <QTime>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <iostream>
#include <memory>
#include "Solver.h"
#include "Collision.h"

Solver* Solver::_instance = nullptr;

Solver::Solver()
{
    if(_instance != nullptr)
    {
        std::abort();
    }

    _instance = this;

    _node = new osg::Group;

    _numBodies = 0;
    _dim = 0;

    _timestep = 1000 / 60;

    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(step()));

    _gravity = Eigen::Vector3d{0.0, 0.0, -9.81};
    _linearViscosity = 5.0e3;
    _angularViscosity = 1.0e5;

    // create axes node.

    {
        const double r = 0.1;
        const double l = 5.0;

        osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere( osg::Vec3(0.0, 0.0, 0.0), 2.0*r );
        osg::ref_ptr<osg::Cylinder> cylx = new osg::Cylinder( osg::Vec3(0.5*l, 0.0, 0.0), r, l);
        osg::ref_ptr<osg::Cylinder> cyly = new osg::Cylinder( osg::Vec3(0.0, 0.5*l, 0.0), r, l);
        osg::ref_ptr<osg::Cylinder> cylz = new osg::Cylinder( osg::Vec3(0.0, 0.0, 0.5*l), r, l);
        cylx->setRotation( osg::Quat(0.0, -M_SQRT2*0.5, 0.0, M_SQRT2*0.5) );
        cyly->setRotation( osg::Quat(-M_SQRT2*0.5, 0.0, 0.0, M_SQRT2*0.5) );

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable( new osg::ShapeDrawable( sphere )); 
        geode->addDrawable( new osg::ShapeDrawable( cylx )); 
        geode->addDrawable( new osg::ShapeDrawable( cyly )); 
        geode->addDrawable( new osg::ShapeDrawable( cylz )); 

        _node->addChild(geode);
    }
}

Solver::~Solver()
{
    if(_instance == nullptr)
    {
        std::abort();
    }

    _instance = nullptr;
}

void Solver::addBody(BodyPtr body)
{
    _bodies.push_back( body );
    _node->addChild( body->getRepresentation() );

    body->setId(_numBodies);
    _numBodies++;
    _dim = _numBodies*13;
}

void Solver::addSpring( SpringPtr spring )
{
    _springs.push_back( spring );
    _node->addChild( spring->getRepresentation() );
}

void Solver::init()
{
    for(BodyPtr& b : _bodies)
    {
        b->currentState() = b->initialState();
    }
    syncRepresentation();
}

void Solver::startSimulation()
{
    _timer->start(_timestep);
}

void Solver::stopSimulation()
{
    _timer->stop();
}

void Solver::syncRepresentation()
{
   for(BodyPtr& b : _bodies)
   {
      b->syncRepresentation();
   }

   for(SpringPtr& s : _springs)
   {
      s->syncRepresentation();
   }
}

void Solver::step()
{
    bool go_on = true;
    double time_left = double(_timestep) * 1.0e-3;
    int num_iterations = 0;

    while(go_on)
    {
        //CrankNicholsonMethod method(this);
        ExplicitEulerMethod method(this);

        bool completed;
        double effectivedt;
        method.run(time_left, effectivedt, completed);

        go_on = !completed;
        time_left -= effectivedt;

        // detect collisions.

        /*
        for(int i=0; i<_bodies.size(); i++)
        {
            BodyPtr b1 = _bodies[i];
            b1->representationState() = b1->collisionDetectionState();
            if( b1->isMoving() )
            {
                bool collision = false;

                for(int j=0; collision == false && j<_bodies.size(); j++)
                {
                    if(j != i)
                    {
                        BodyPtr b2 = _bodies[j];

                        Eigen::Vector3d collision_point;

                        collision = Collision::detect(
                            b1.get(),
                            b2.get(),
                            collision_point);
                    }
                }

                if(collision)
                {
                    ;
                }
                else
                {
                    b1->representationState() = b1->collisionDetectionState();
                }
            }
        }
        */

        num_iterations++;
    }

    std::cout << "num_iterations = " << num_iterations << std::endl;

    syncRepresentation();
}

void Solver::retrieveCurrentState(Eigen::VectorXd& X)
{
    X.resize( _dim );

    for(int i=0; i<_numBodies; i++)
    {
        Body::State& s = _bodies[i]->currentState();

        X.segment<3>(13*i+0) = s.position;
        X.segment<4>(13*i+3) = s.attitude.coeffs();
        X.segment<3>(13*i+7) = s.linear_momentum;
        X.segment<3>(13*i+10) = s.angular_momentum;
    }
}

void Solver::normalizeState(Eigen::VectorXd& X)
{
    assert( X.size() == _dim );

    for(int i=0; i<_numBodies; i++)
    {
        // we could just do X.segment<4>(13*i+3).normalize().
        Eigen::Quaterniond u;
        u.coeffs() = X.segment<4>(13*i+3);
        u.normalize();
        X.segment<4>(13*i+3) = u.coeffs();
    }
}

void Solver::applyState(const Eigen::VectorXd& X)
{
    assert( X.size() == _dim );

    for(int i=0; i<_numBodies; i++)
    {
        BodyPtr body = _bodies[i];
        if(body->isMoving())
        {
            body->currentState() = extractIndividualState(X, i);
        }
    }
}

void Solver::computeStateDerivative(const Eigen::VectorXd& X, Eigen::VectorXd& f)
{
    assert( X.size() == _dim );

    f.resize(_dim);

    for(int i=0; i<_numBodies; i++)
    {
        BodyPtr body = _bodies[i];

        if(body->isFixed())
        {
            f.segment<13>(13*i).setZero();
        }
        else
        {
            Body::State state = extractIndividualState(X, i);

            const Eigen::Vector3d linear_velocity = state.linear_momentum / body->getMass();
            const Eigen::Vector3d angular_velocity = body->getInertiaTensorSolver().solve( state.angular_momentum );

            Eigen::Vector3d resultant_force = Eigen::Vector3d::Zero();
            Eigen::Vector3d resultant_torque = Eigen::Vector3d::Zero();

            resultant_force += body->getMass() * _gravity;
            resultant_force -= _linearViscosity * linear_velocity;
            resultant_torque -= _angularViscosity * angular_velocity;

            const int id = body->getId();

            f.segment<3>(13*id+0) = linear_velocity;
            f.segment<4>(13*id+3) = 0.5 * ( state.attitude * Eigen::Quaterniond(0.0, angular_velocity.x(), angular_velocity.y(), angular_velocity.z()) ).coeffs();
            f.segment<3>(13*id+7) = resultant_force;
            f.segment<3>(13*id+10) = resultant_torque;
        }
    }

    for(SpringPtr spring : _springs)
    {
        Body* B1 = spring->getBody1();
        Body* B2 = spring->getBody2();

        const int id1 = B1->getId();
        const int id2 = B2->getId();

        Body::State S1 = extractIndividualState(X, id1);
        Body::State S2 = extractIndividualState(X, id2);

        const Eigen::Vector3d P1 = S1.position + S1.attitude * spring->getAnchor1();
        const Eigen::Vector3d P2 = S2.position + S2.attitude * spring->getAnchor2();

        const double L = (P2 - P1).norm();
        const double cte = spring->getElasticityCoefficient() * (L - spring->getFreeLength()) / spring->getFreeLength();
        const Eigen::Vector3d F = (P2-P1).normalized() * cte;

        if(B1->isMoving())
        {
            f.segment<3>(13*id1+7) += F;
            f.segment<3>(13*id1+10) += spring->getAnchor1().cross( B1->currentState().attitude.inverse() * F );
        }

        if(B2->isMoving())
        {
            f.segment<3>(13*id2+7) -= F;
            f.segment<3>(13*id2+10) -= spring->getAnchor2().cross( B2->currentState().attitude.inverse() * F );
        }
    }
}

Body::State Solver::extractIndividualState(const Eigen::VectorXd& X, int id)
{
    Body::State s;

    s.position = X.segment<3>(13*id+0);
    s.attitude.coeffs() = X.segment<4>(13*id+3);
    s.attitude.normalize();
    s.linear_momentum = X.segment<3>(13*id+7);
    s.angular_momentum = X.segment<3>(13*id+10);

    return s;
}

void Solver::computeTimestep(const Eigen::VectorXd& X, double maxdt, double& dt, bool& completed)
{
    completed = true;
    dt = maxdt;

    for(int i=0; i<_numBodies; i++)
    {
        BodyPtr body = _bodies[i];
        Body::State state = extractIndividualState(X, i);

        const Eigen::Vector3d linear_velocity = state.linear_momentum / body->getMass();
        const Eigen::Vector3d angular_velocity = body->getInertiaTensorSolver().solve( state.angular_momentum );

        const double bs_radius = body->getBoundingSphere().radius;
        const double max_dist = bs_radius * 0.01;
        const double dt1 = max_dist / linear_velocity.norm();
        const double dt2 = max_dist / std::max(1.0e-5, angular_velocity.norm()*bs_radius);
        const double dt3 = std::min(dt1, dt2);

        if(dt3 < dt)
        {
            completed = false;
            dt = dt3;
        }
    }
}

// Crank-Nicholson method for solving the ODE.

Solver::CrankNicholsonMethod::CrankNicholsonMethod(Solver* solver, double theta)
{
    _theta = theta;
    _solver = solver;
}

void Solver::CrankNicholsonMethod::run(double maxdt, double& dt, bool& completed)
{
    const int d = _solver->_dim;

    Eigen::VectorXd f;
    Eigen::VectorXd F0;
    Eigen::VectorXd X;

    _solver->retrieveCurrentState(X);
    _solver->computeTimestep(X, maxdt, dt, completed);
    _solver->computeStateDerivative(X, f);
    F0 = (1.0/dt) * X + _theta*f;

    bool go_on = true;
    while(go_on)
    {
        // compute RHS of the linear system.

        Eigen::VectorXd RHS = F0 - (1.0/dt)*X + (1.0-_theta)*f;

        // compute the coefficients of the jacobian matrix.

        std::vector< Eigen::Triplet<double> > triplets;
        //triplets.reserve(d + _solver->_numBodies*(3+3+4+

        for(int i=0; i<d; i++)
        {
            triplets.push_back( Eigen::Triplet<double>(i, i, 1.0/dt) );
        }

        const double cte = -(1.0-_theta);
        for(int i=0; i<_solver->_numBodies; i++)
        {
            BodyPtr body = _solver->_bodies[i];
            Body::State state = _solver->extractIndividualState(X, i);

            //triplets.push;
        }

        for(SpringPtr spring : _solver->_springs)
        {
            ;
        }

        // build the jacobian matrix.

        Eigen::SparseMatrix<double> gradient(d, d);
        gradient.setFromTriplets(triplets.begin(), triplets.end());

        // find dX by resolving the linear system.

        Eigen::ConjugateGradient< Eigen::SparseMatrix<double> > cg;
        cg.compute(gradient);
        Eigen::VectorXd dX = cg.solve(RHS);

        // update the state.

        X += dX;
        _solver->normalizeState(X);

        // test if we continue.

        go_on = false;
        /*
        for(int i=0; go_on == false && i<_solver->_numBodies; i++)
        {
            const double bsradius = _solver->_bodies[i]->getBoundingBox().radius;

            Body::State s1 = _solver->extractIndividualState(X0, i);
            Body::State s2 = _solver->extractIndividualState(X, i);

            go_on = go_on || (s2.position - s1.position).norm() > 0.01*bsradius;
            go_on = go_on || 
        }
        */
        go_on = dX.lpNorm<Eigen::Infinity>() > 0.01;

        if(go_on)
        {
            _solver->computeStateDerivative(X, f);
        }
    }

    _solver->applyState(X);
}

// Explicit Euler method.

Solver::ExplicitEulerMethod::ExplicitEulerMethod(Solver* solver)
{
    _solver = solver;
}

void Solver::ExplicitEulerMethod::run(double maxdt, double& dt, bool& completed)
{
    completed = true;
    dt = maxdt;

    Eigen::VectorXd X;
    _solver->retrieveCurrentState(X);

    _solver->computeTimestep(X, maxdt, dt, completed);

    Eigen::VectorXd f;
    _solver->computeStateDerivative(X, f);

    X += dt*f;

    _solver->normalizeState(X);
    _solver->applyState(X);
}

