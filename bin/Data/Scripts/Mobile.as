class Mobile : ScriptObject
{
    Node@ player;
    Array<Vector3> currentPath;
    bool Moving = false;
    float pathDelta = 1000;
    float rayDelta = 1000;
    DynamicNavigationMesh@ navMesh;

    void Start()
    {
        navMesh = node.scene.GetComponent("DynamicNavigationMesh");
    }

    void Update(float timeStep)
    {
        player = node.scene.GetChild("Camera", true);

        Vector3 ownPos = navMesh.FindNearestPoint(node.position, Vector3(1.0f, 3.5f, 1.0f));
        Vector3 targetPos(player.position + Vector3(0, 0, 0));
        //Vector3 mdir = targetPos - ownPos;
        //mdir.Normalize();

        float distance = (targetPos - ownPos).length;

        if(distance > 5)
        {
            if(!Moving || pathDelta > 5)
            {
                Print("Computing Path...");
                Vector3 pathPos = navMesh.FindNearestPoint(targetPos, Vector3(1.0f, 3.5f, 1.0f));
                currentPath = navMesh.FindPath(ownPos, pathPos);

                if(currentPath.length < 1)
                {
                    Print("Target is not on ground: " + pathPos.x + ", " + pathPos.y + ", " + pathPos.z);

                    RayQueryResult result = node.scene.octree.RaycastSingle( Ray( Vector3(targetPos.x, targetPos.y - 0.5, targetPos.z), Vector3(0.0f, -1.0f, 0.0f) ), RAY_TRIANGLE, 20, DRAWABLE_GEOMETRY);
                    if(result.drawable !is null)
                    {
                        Vector3 hitPos = result.position;
                        Print("Trying: " + hitPos.x + ", " + hitPos.y + ", " + hitPos.z);

                        pathPos = navMesh.FindNearestPoint(hitPos, Vector3(1.0f, 3.5f, 1.0f));
                        currentPath = navMesh.FindPath(ownPos, pathPos);
                        if(currentPath.length < 1)
                        {
                            Print("No path to target: " + pathPos.x + ", " + pathPos.y + ", " + pathPos.z);
                        }
                    }
                }
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
            Vector3 nextPath = currentPath[0]; // NB: currentPath[0] is the next waypoint in order
            //Vector3 nextWaypoint = navMesh.MoveAlongSurface(node.position, nextPath, Vector3(0.5f, 1.5f, 0.5f), 10);
            Vector3 nextWaypoint = currentPath[0]; // NB: currentPath[0] is the next waypoint in order

            Vector3 mdir = nextPath - node.position;

            float move = 25.0f * timeStep;
            float distance = (node.position - nextWaypoint).length;
            if (move > distance)
                move = distance;

            node.LookAt(nextWaypoint, Vector3(0.0f, 1.0f, 0.0f));
            node.Translate(Vector3(0.0f, 0.0f, 1.0f) * move);

            if(rayDelta > 0.1)
            {
                RayQueryResult result = node.scene.octree.RaycastSingle( Ray( Vector3(node.position.x, node.position.y + 2, node.position.z), Vector3(0.0f, -1.0f, 0.0f) ), RAY_TRIANGLE, 10, DRAWABLE_GEOMETRY);
                if(result.drawable !is null)
                {
                    Vector3 hitPos = result.position;
                    //node.Translate( Vector3(0.0f, 0.0f, hitPos.z - node.position.z) );
                    node.position = result.position;
                }
                rayDelta = 0;
            }
            rayDelta += timeStep;

            //Vector3 zPos = navMesh.FindNearestPoint(node.position, Vector3(0.5f, 1.5f, 0.5f));
            //RigidBody@ rb = node.GetComponent("RigidBody");

            //rb.linearVelocity = Vector3(0,0,0);
            //rb.ApplyImpulse(mdir);

            // Remove waypoint if reached it
            if (distance < 0.1)
            {
                currentPath.Erase(0);
                //rb.linearVelocity = Vector3(0,0,0);
            }
        }
        else
        {
            //Print("No Path!");
        }
    }

}
