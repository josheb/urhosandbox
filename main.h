#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <cstdlib>
#include <map>

#include "Character.h"

#ifdef _MSC_VER
#include <cstdio>
#define snprintf _snprintf
#endif

using namespace Urho3D;

class MyApp : public Application
{
public:
    SharedPtr<Scene> scene_;
    SharedPtr<PhysicsWorld> pw_;
    Vector3 up_;
    Node * cameraNode;
    float yaw_;
    float pitch_;
    Console *console;
    DebugHud *debughud;
    float walkspeed;
    bool flymode;
    WeakPtr<Character> character_;

    /************************/

    MyApp(Context * context);

    virtual void Setup();
    virtual void Start();
    virtual void Stop();
    void HandleBeginFrame(StringHash eventType, VariantMap & eventData);
    void HandleKeyDown(StringHash eventType, VariantMap & eventData);
    void UIMouse(bool on);
    void HandleControlClicked(StringHash eventType, VariantMap& eventData);
    void HandleMouseUp(StringHash eventType, VariantMap& eventData);
    void HandleUpdate(StringHash eventType, VariantMap & eventData);
    void HandlePostUpdate(StringHash eventType, VariantMap & eventData);
    void HandleRenderUpdate(StringHash eventType, VariantMap & eventData);
    void HandlePostRenderUpdate(StringHash eventType, VariantMap & eventData);
    void HandleEndFrame(StringHash eventType, VariantMap & eventData);
    void ShootObject(void);
    void MovePlayer(float timeStep);
    void MoveCamera(float timeStep);
    void HandleCollideStart(StringHash eventType, VariantMap& eventData);
    void HandleConsoleCommand(StringHash eventType, VariantMap& eventData);
    void HandleInput(const String& input);
    void Print(const String& output);
};
