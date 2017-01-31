class Mobile : ScriptObject
{
    Node@ player;

    void Update(float timeStep)
    {
        player = node.scene.GetChild("Camera", true);

        Vector3 ownPos(node.position + Vector3(0, 0.5, 0));
        Vector3 targetPos(player.position + Vector3(0, 0.5, 0));
        Vector3 mdir = targetPos - ownPos;
        //mdir.Normalize();

        float distance = (targetPos - ownPos).length;

        RigidBody@ rb = node.GetComponent("RigidBody");

        if(distance > 5)
        {
            rb.linearVelocity = Vector3(0,0,0);
            rb.ApplyImpulse(mdir);
        }
        else
        {
            rb.linearVelocity = Vector3(0,0,0);
        }

        //Vector3 rotationSpeed = Vector3(50, 50, 0);
        //node.Rotate(Quaternion(rotationSpeed.x * timeStep, rotationSpeed.y * timeStep, rotationSpeed.z * timeStep));
    }
}
