#ifndef _MOTION_PLANNING_LIBRARIES_HPP_
#define _MOTION_PLANNING_LIBRARIES_HPP_

#include <base/samples/RigidBodyState.hpp>
#include <base/Waypoint.hpp>
#include <base/Trajectory.hpp>

#include <envire/maps/TraversabilityGrid.hpp>

#include "Config.hpp"
#include "State.hpp"
#include "AbstractMotionPlanningLibrary.hpp"

namespace motion_planning_libraries
{

typedef envire::TraversabilityGrid::ArrayType TravData;

enum MplErrors {
    MPL_ERR_NONE,
    MPL_ERR_UNDEFINED,
    MPL_ERR_MISSING_START_STATE,
    MPL_ERR_MISSING_GOAL_STATE,
    MPL_ERR_MISSING_TRAV_GRID,
    MPL_ERR_PLANNING_FAILED,
    MPL_ERR_WRONG_STATE_TYPE,
    MPL_ERR_INITIALIZE_MAP,
    MPL_ERR_SET_STATES
};
    
/**
 * Allows x,y,theta path planning for navigation and motion planning for arm control.
 * Path planning is meant to work on a single traversability map.
 * The start/goal poses are transformed from world to local grid and the
 * resulting trajectory from local grid to world.
 * SBPL should be used for navigation and OMPL for arm control.
 * 
 * TODO: Integrate: remove obstacles within the start position, 
 *       if the goal lies on an obstacle use next valid goal position
 * TODO: Split State and Config to classes using inheritance.
 */
class MotionPlanningLibraries
{
 protected:
    static const double REPLANNING_DIST_THRESHOLD = 0.05;
    static const double REPLANNING_TURN_THRESHOLD = 0.017;
    static const double REPLANNING_JOINT_ANGLE_THRESHOLD = 0.017;
    
    Config mConfig;
    
    boost::shared_ptr<AbstractMotionPlanningLibrary> mpPlanningLib;
    
    envire::TraversabilityGrid* mpTravGrid;
    boost::shared_ptr<TravData> mpTravData;
    struct State mStartState, mGoalState; // Pose in world coordinates.
    struct State mStartStateGrid, mGoalStateGrid;
    std::vector<State> mPlannedPathInWorld; // Pose in world coordinates.
    bool mReceivedNewTravGrid;
    bool mReceivedNewStart;
    bool mReceivedNewGoal;
    bool mArmInitialized;
    bool mReplanRequired;
    
    enum MplErrors mError;
    
 public: 
    MotionPlanningLibraries(Config config = Config());
    ~MotionPlanningLibraries();
    
    /**
     * Sets the traversability map to plan on. Required for robot navigation.
     * The pose, scale and size of the map are used for the world2grid and
     * grid2world transformation.
     */
    bool setTravGrid(envire::Environment* env, std::string trav_map_id);
    
    /**
     * Sets the start state. If the start state contains a pose it has to be defined 
     * within the world frame. This pose is transformed to the traversability 
     * grid and is used to set mStartGrid.
     */
    bool setStartState(struct State start_state);
    
    /**
     * Sets the goal state. If the goal state contains a pose it has to be defined 
     * within the world frame. This pose is transformed to the traversability 
     * grid and is used to set mGoalGrid.
     */
    bool setGoalState(struct State goal_state);
    
    /**
     * Tries to find a trajectory within the passed time.
     * If this method is called several times (with the same configurations),
     * the planner will try to improve the found path. Otherwise a new planning
     * will be initiated. Thresholds define whether a state is new one.
     * For navigation: A new traversability map will initiate a reinitialization.
     * TODO: A new trav map should not require a complete reinitialization.
     */
    bool plan(double max_time=1.0); 
    
    /**
     * Like getStates() but with world coordinates.
     */
    std::vector<struct State> getStatesInWorld();
    
    // POSE SPECIFIC METHODS.
    /** Returns the path stored in mPath as a list of waypoints. */
    std::vector<base::Waypoint> getPathInWorld();
    
    /** 
     * Returns the path stored in mPath as a trajectory (spline). 
     * Currently the path is divided into forward and backward movements.
     */
    base::Trajectory getTrajectoryInWorld(double speed);
    
    /** Prints the current path to the console. */
    void printPathInWorld();
    
    /** Returns the start pose within the grid. */
    base::samples::RigidBodyState getStartPoseInGrid();
    
    /** Returns the goal pose within the grid. */
    base::samples::RigidBodyState getGoalPoseInGrid();
    
    /** If an error occurred during planning this can be used to get more informations. */
    inline enum MplErrors getError() {
        return mError;
    }
    
    bool getSbplMotionPrimitives(struct SbplMotionPrimitives& prims);
    
    /**
     * Converts the world pose to grid coordinates including the transformed orientation.
     */        
    static bool world2grid(envire::TraversabilityGrid const* trav, 
        base::samples::RigidBodyState const& world_pose, 
        base::samples::RigidBodyState& grid_pose);
        
    /**
     * Transforms the grid-coordinates to grid local to world.
     */
    static bool grid2world(envire::TraversabilityGrid const* trav,
            base::samples::RigidBodyState const& grid_pose, 
            base::samples::RigidBodyState& world_pose);
     
    /**
     * Transforms the pose defined in the grid local frame to world.
     * Poses within the grid local frame use meter and radians (-PI, PI] instead of
     * discrete grid coordinates.
     */
    static bool gridlocal2world(envire::TraversabilityGrid const* trav,
        base::samples::RigidBodyState const& grid_local_pose,
        base::samples::RigidBodyState& world_pose);
    
 private:
    /**
     * Extracts the traversability map \a trav_map_id from the passed environment.
     * If the id is not available, the first traversability map will be used.
     */
    envire::TraversabilityGrid* extractTravGrid(envire::Environment* env, 
            std::string trav_map_id);
};

} // end namespace motion_planning_libraries

#endif // _MOTION_PLANNING_LIBRARIES_HPP_
