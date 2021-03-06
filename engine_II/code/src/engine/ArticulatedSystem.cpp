#include <stack>
#include "ArticulatedSystem.h"

template<int D>
ArticulatedSystem<D>::ArticulatedSystem()
{
}

template<int D>
void ArticulatedSystem<D>::init()
{
    mLinkOffset.resize(mBodiesVector.size());
    mNumDegreesOfFreedom = 0;

    for(size_t id=0; id<mBodiesVector.size(); id++)
    {
        mBodiesVector[id]->setId(id);
        mLinkOffset[id] = mNumDegreesOfFreedom;
        mNumDegreesOfFreedom += mBodiesVector[id]->getJoint()->getNumDegreesOfFreedom();
    }
}

template<int D>
size_t ArticulatedSystem<D>::getNumDegreesOfFreedom()
{
    return mNumDegreesOfFreedom;
}

template<int D>
size_t ArticulatedSystem<D>::getNumBodies()
{
    return mBodiesVector.size();
}

template<int D>
std::vector< ArticulatedBodyPtr<D> >& ArticulatedSystem<D>::refBodiesTree()
{
    return mBodiesTree;
}

template<int D>
std::vector< ArticulatedBodyPtr<D> >& ArticulatedSystem<D>::refBodiesVector()
{
    return mBodiesVector;
}

template<int D>
BodyPtr<D> ArticulatedSystem<D>::getBody(size_t i)
{
    return mBodiesVector[i];
}

template<int D>
void ArticulatedSystem<D>::evaluate(
    double t,
    const Eigen::VectorXd& state,
    typename MechanicalSystem<D>::Pose0Tensor& poses,
    typename MechanicalSystem<D>::Pose1Tensor& first_order_derivatives,
    typename MechanicalSystem<D>::Pose2Tensor& second_order_derivatives)
{
/*
    const size_t F = getNumDegreesOfFreedom();
    const size_t N = getNumBodies();

    poses = Pose0Tensor({ N, D, D+1 });
    first_order_derivatives = Pose1Tensor({ N, D, D+1, F });
    second_order_derivatives = Pose2Tensor({ N, D, D+1, F, F });

    std::vector< Pose<D> > body_to_world(N);

    std::stack<BodyPtr<D>> stack;

    for( BodyPtr<D> body : mBodiesTree )
    {
        stack.push(body);
    }

    while( stack.empty() == false )
    {
        BodyPtr<D> body = stack.top();
        stack.pop();
        */
        /*
        LinkPtr<D> link = body->getLink();

        const Eigen::VectorXd link_state = state.segment( mLinkOffset[body->getId()], link->getNumDegreesOfFreedom() );

        Pose<D> child_to_parent;

        body->getLink()->evaluate(link_state, child_to_parent);
        */
        /*
    }
*/
}

template class ArticulatedSystem<2>;
template class ArticulatedSystem<3>;

