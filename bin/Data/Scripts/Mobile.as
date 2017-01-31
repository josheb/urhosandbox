class Mobile : ScriptObject
{
    Node@ player;
    Array<Vector3> currentPath;
    bool Moving = false;
    float pathDelta = 1000;
    DynamicNavigationMesh@ navMesh;

    void Start()
    {
        navMesh = node.scene.GetComponent("DynamicNavigationMesh");
    }

    void Update(float timeStep)
    {
        player = node.scene.GetChild("Camera", true);

        Vector3 ownPos(node.position + Vector3(0, 0.5, 0));
        Vector3 targetPos(player.position + Vector3(0, 0.5, 0));
        Vector3 mdir = targetPos - ownPos;
        //mdir.Normalize();

        float distance = (targetPos - ownPos).length;

        if(distance > 5)
        {
            if(!Moving || pathDelta > 5)
            {
                Print("Computing Path...");
                Vector3 pathPos = navMesh.FindNearestPoint(targetPos, Vector3(1.0f, 1.0f, 1.0f));
                currentPath = navMesh.FindPath(node.position, pathPos);
                pathDelta = 0;
                Moving = true;
            }

            FollowPath(timeStep);
        }
        else
        {
            Moving = false;
        }

        pathDelta += timeStep;
    }

    void FollowPath(float timeStep)
    {
        if (currentPath.length > 0)
        {
            Vector3 nextWaypoint = currentPath[0]; // NB: currentPath[0] is the next waypoint in order

            float move = 25.0f * timeStep;
            float distance = (node.position - nextWaypoint).length;
            if (move > distance)
                move = distance;

            node.LookAt(nextWaypoint, Vector3(0.0f, 1.0f, 0.0f));
            node.Translate(Vector3(0.0f, 0.0f, 1.0f) * move);

            // Remove waypoint if reached it
            if (distance < 0.1)
                currentPath.Erase(0);
        }
        else
        {
            //Print("No Path!");
        }
    }

}
