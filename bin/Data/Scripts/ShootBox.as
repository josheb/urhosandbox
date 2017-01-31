class ShootBox : ScriptObject
{
    Vector3 rotationSpeed;

    float time = 0;

    void SetRotationSpeed(const Vector3&in speed)
    {
        rotationSpeed = speed;
    }

    // Update is called during the variable timestep scene update
    void Update(float timeStep)
    {
        time += timeStep;

        rotationSpeed = Vector3(50, 50, 0);
        node.Rotate(Quaternion(rotationSpeed.x * timeStep, rotationSpeed.y * timeStep, rotationSpeed.z * timeStep));

        if(time > 10) { node.Remove(); }
    }
}
