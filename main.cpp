#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/AngelScript/Script.h>
#include <Urho3D/AngelScript/ScriptFile.h>
#include <Urho3D/AngelScript/ScriptInstance.h>
#include <Urho3D/Navigation/Navigable.h>
#include <Urho3D/Navigation/NavigationMesh.h>
#include <Urho3D/Navigation/DynamicNavigationMesh.h>
#include <cstdlib>
#include <map>
#include <RakPeer.h>
#include <RakPeerInterface.h>

#include "main.h"
#include "Character.h"

#ifdef _MSC_VER
#include <cstdio>
#define snprintf _snprintf
#endif

using namespace Urho3D;

class Character;

MyApp::MyApp(Context * context) : Application(context)
{
    Character::RegisterObject(context);
    context->RegisterSubsystem(new Script(context));
    flymode = false;
}

void MyApp::Setup()
{
    engineParameters_["FullScreen"]  = false;
    engineParameters_["WindowWidth"]  = 1440;
    engineParameters_["WindowHeight"]  = 900;
    engineParameters_["ResourcePrefixPath"]  = "";
    walkspeed = 100;
}

void MyApp::Start()
{
    GetSubsystem<Input>()->SetMouseVisible(false);
    GetSubsystem<Input>()->SetMouseGrabbed(true);

    up_ = Vector3::UP;

    ResourceCache * cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    //scene_->CreateComponent<Octree>();
    //scene_->CreateComponent<DebugRenderer>();

    //SharedPtr<File> file = cache->GetFile("Scenes/dungeon1.xml");
    SharedPtr<File> file = cache->GetFile("Scenes/ttest2.xml");
    //SharedPtr<File> file = cache->GetFile("Scenes/SceneLoadExample.xml");
    scene_->LoadXML(*file);

    UI* ui = GetSubsystem<UI>();
    GetSubsystem<UI>()->GetRoot()->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));

    SharedPtr<UIElement> layoutRoot = ui->LoadLayout(cache->GetResource<XMLFile>("UI/crosshair.xml"));
    ui->GetRoot()->AddChild(layoutRoot);

    cameraNode = scene_->CreateChild("Camera");
    Camera * camera = cameraNode->CreateComponent<Camera>();
    camera->SetFarClip(65000.0f);
    camera->SetFov(90);

    Light * light = cameraNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_POINT);
    light->SetRange(0.0f);
    light->SetBrightness(1.0);
    light->SetSpecularIntensity(10);
    light->SetColor(Color(1.0, 1.0, 1.0, 1.0));

    Renderer * renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

    console = engine_->CreateConsole();
    console->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
    console->GetBackground()->SetOpacity(0.8f);

    console->SetNumRows( (GetSubsystem<Graphics>()->GetHeight() / 2) / 16);
    console->SetNumBufferedRows(2 * console->GetNumRows());
    console->SetCommandInterpreter(GetTypeName());
    console->SetVisible(false);
    console->GetCloseButton()->SetVisible(false);

    // Create debug HUD.
    debughud = engine_->CreateDebugHud();
    debughud->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
    debughud->SetUseRendererStats(1);

    //Find a starting position in the map
    Node* camstart = scene_->GetChild("CamStart", true);
    //cameraNode->SetPosition(camstart->GetPosition());
    const Variant heading = camstart->GetVar("Heading");
    yaw_ = heading.GetFloat();

    MoveCamera(0);

    SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(MyApp, HandleBeginFrame));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(MyApp, HandleKeyDown));
    SubscribeToEvent(E_UIMOUSECLICK, URHO3D_HANDLER(MyApp, HandleControlClicked));
    SubscribeToEvent(E_MOUSEBUTTONUP, URHO3D_HANDLER(MyApp, HandleMouseUp));
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(MyApp, HandleUpdate));
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(MyApp, HandlePostUpdate));
    SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(MyApp, HandleRenderUpdate));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(MyApp, HandlePostRenderUpdate));
    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(MyApp, HandleEndFrame));
    SubscribeToEvent(E_CONSOLECOMMAND, URHO3D_HANDLER(MyApp, HandleConsoleCommand));
    SubscribeToEvent(E_PHYSICSCOLLISIONSTART, URHO3D_HANDLER(MyApp, HandleCollideStart));

    Node* swordNode = scene_->CreateChild("Sword");
    StaticModel * swo = swordNode->CreateComponent<StaticModel>();
    swo->SetModel(cache->GetResource<Model>("Models/swordtest.mdl"));
    swo->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
    swo->SetCastShadows(true);

    RigidBody* swb = swordNode->CreateComponent<RigidBody>();
    swb->SetCollisionLayer(5);
    swb->SetMass(0.0f);
    swb->SetAngularFactor(Vector3::ZERO);
    swb->SetLinearFactor(Vector3::ZERO);
    CollisionShape* sws = swordNode->CreateComponent<CollisionShape>();
    sws->SetTriangleMesh(cache->GetResource<Model>("Models/swordtest.mdl"));

    Node* objectNode = scene_->CreateChild("Jack");
    objectNode->SetScale(0.5f);
    //objectNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));
    objectNode->SetPosition(camstart->GetPosition());

    // Create the rendering component + animation controller
    AnimatedModel* torso = objectNode->CreateComponent<AnimatedModel>();
    torso->SetModel(cache->GetResource<Model>("Models/Torso.mdl"));
    torso->SetMaterial(cache->GetResource<Material>("Materials/male1.xml"));
    torso->SetCastShadows(true);

    //chest foot hands head legs
    AnimatedModel* arms = objectNode->CreateComponent<AnimatedModel>();
    arms->SetModel(cache->GetResource<Model>("Models/Arms.mdl"));
    arms->SetMaterial(cache->GetResource<Material>("Materials/male1.xml"));
    arms->SetCastShadows(true);
    AnimatedModel* legs = objectNode->CreateComponent<AnimatedModel>();
    legs->SetModel(cache->GetResource<Model>("Models/Legs.mdl"));
    legs->SetMaterial(cache->GetResource<Material>("Materials/male1.xml"));
    legs->SetCastShadows(true);

    //AnimatedModel* feet = objectNode->CreateComponent<AnimatedModel>();
    //feet->SetModel(cache->GetResource<Model>("Models/varsoiM_foot.mdl"));
    //feet->SetMaterial(cache->GetResource<Material>("Materials/male1.xml"));
    //feet->SetCastShadows(true);

    AnimatedModel* head = objectNode->CreateComponent<AnimatedModel>();
    head->SetModel(cache->GetResource<Model>("Models/Head.mdl"));
    head->SetMaterial(cache->GetResource<Material>("Materials/male1.xml"));
    head->SetCastShadows(true);

    objectNode->CreateComponent<AnimationController>();

    // Set the head bone for manual control
    //object->GetSkeleton().GetBone("Bip01_Head")->animated_ = false;
    //object->GetSkeleton().GetBone("head")->animated_ = false;

    //Node *hn = objectNode->GetChild("Bip01_R_Finger1", true);
    //hn->AddChild(swordNode);

    swordNode->Pitch(-90);
//        swordNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));
    swordNode->SetName("swordnode");

    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
    RigidBody* body = objectNode->CreateComponent<RigidBody>();
    //body->SetCollisionLayer(1);
    body->SetMass(1.0f);

    // Set zero angular factor so that physics doesn't turn the character on its own.
    // Instead we will control the character yaw manually
    body->SetAngularFactor(Vector3::ZERO);

    // Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
    body->SetCollisionEventMode(COLLISION_ALWAYS);

    // Set a capsule shape for collision
    CollisionShape* shape = objectNode->CreateComponent<CollisionShape>();
    shape->SetCapsule(0.9f, 4.0f, Vector3(0.0f, 2.0f, 0.0f));

    // Create the character logic component, which takes care of steering the rigidbody
    // Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
    // and keeps it alive as long as it's not removed from the hierarchy
    character_ = objectNode->CreateComponent<Character>();

    pw_ = scene_->GetComponent<PhysicsWorld>();
    pw_->SetGravity( Urho3D::Vector3(0.0, -9.8, 0.0) );

    RakNet::RakPeer *rpi = new RakNet::RakPeer();

    DynamicNavigationMesh* navMesh = scene_->GetComponent<DynamicNavigationMesh>();
    if(navMesh)
    {
        navMesh->Build();
    }
    else
    {
        Print("[NavMesh] - No navmesh in scene!");
    }

    OpenConsoleWindow();
}

void MyApp::Stop()
{
    GetSubsystem<Input>()->SetMouseVisible(true);
    GetSubsystem<Input>()->SetMouseGrabbed(false);
}

void MyApp::HandleBeginFrame(StringHash eventType, VariantMap & eventData)
{
}

void MyApp::HandleKeyDown(StringHash eventType, VariantMap & eventData)
{
    using namespace KeyDown;
    int key = eventData[P_KEY].GetInt();

    //if (key == KEY_ESC) { engine_->Exit(); }
    if(key == KEY_F1)
    {
        if(console->IsVisible())
        {
            UIMouse(false);
            console->SetVisible(false);
        }
        else
        {
            UIMouse(true);
            console->SetVisible(true);
        }
    }

    if(key == 'F' || key == KEY_F2)
    {
        flymode = !flymode;
    }
}

void MyApp::UIMouse(bool on)
{
    if(on)
    {
        GetSubsystem<Input>()->SetMouseVisible(true);
        GetSubsystem<Input>()->SetMouseGrabbed(false);
    }
    else
    {
        GetSubsystem<Input>()->SetMouseVisible(false);
        GetSubsystem<Input>()->SetMouseGrabbed(true);
    }

}

void MyApp::HandleControlClicked(StringHash eventType, VariantMap& eventData)
{
    //engine_->Exit();
}

void MyApp::HandleMouseUp(StringHash eventType, VariantMap& eventData)
{
    int button = eventData[MouseButtonUp::P_BUTTON].GetInt();

    ShootObject();
}

void MyApp::HandleUpdate(StringHash eventType, VariantMap & eventData)
{
    float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

    Graphics* graphics = GetSubsystem<Graphics>();
    Camera* camera = cameraNode->GetComponent<Camera>();
    //Ray cameraRay = camera->GetScreenRay((float)pos.x_ / graphics->GetWidth(), (float)pos.y_ / graphics->GetHeight());
    Ray cameraRay = camera->GetScreenRay( 0.5, 0.5 );

    // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
    // We should store the current hit object so we can give it a 'hover' status.  If results.size() is 0 we should clear the object.
    PODVector<RayQueryResult> results;
    RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, 250, DRAWABLE_GEOMETRY);
    scene_->GetComponent<Octree>()->RaycastSingle(query);
    if (results.Size())
    {
        RayQueryResult& result = results[0];
        //Vector3 hitPos = result.position_;
        Node* hitnode = result.node_;

        RigidBody *rb = reinterpret_cast<RigidBody *>( hitnode->GetComponent("RigidBody") );
        if(rb)
        {
            //rb->ApplyImpulse( Vector3(0.0, 4.0, 0.0) );
        }
    }

    MoveCamera(timeStep);
    MovePlayer(timeStep);

    String input = GetConsoleInput();
    if (input.Length()) { HandleInput(input); }
}

void MyApp::HandlePostUpdate(StringHash eventType, VariantMap & eventData)
{
    if (!character_)
        return;

    Node* characterNode = character_->GetNode();

    // Get camera lookat dir from character yaw + pitch
    Quaternion rot = characterNode->GetRotation();
    Quaternion dir = rot * Quaternion(character_->controls_.pitch_, Vector3::RIGHT);

    // Turn head to camera pitch, but limit to avoid unnatural animation
    //Node* headNode = characterNode->GetChild("Bip01_Head", true);
    Node* headNode = characterNode->GetChild("Head", true);
    float limitPitch = Clamp(character_->controls_.pitch_, -45.0f, 45.0f);
    Quaternion headDir = rot * Quaternion(limitPitch, Vector3(10.0f, 0.0f, 10.0f));
    // This could be expanded to look at an arbitrary target, now just look at a point in front
    Vector3 headWorldTarget = headNode->GetWorldPosition() + headDir * Vector3(0.0f, 0.0f, 1.0f);
    //headNode->LookAt(headWorldTarget, Vector3(0.0f, 1.0f, 0.0f));
    // Correct head orientation because LookAt assumes Z = forward, but the bone has been authored differently (Y = forward)
    //headNode->Rotate(Quaternion(0.0f, 90.0f, 90.0f));

    //For FPS camera
    if(!flymode)
    {
        //cameraNode->SetPosition(headNode->GetWorldPosition() + rot * Vector3(0.0f, 0.15f, 0.3f));
        cameraNode->SetPosition(characterNode->GetWorldPosition() +rot * Vector3(0.f, 1.65f, 0.3f));
        cameraNode->SetRotation(dir);
    }
    pw_->DrawDebugGeometry(true);

    DynamicNavigationMesh* navMesh = scene_->GetComponent<DynamicNavigationMesh>();
    navMesh->DrawDebugGeometry(true);
}

void MyApp::HandleRenderUpdate(StringHash eventType, VariantMap & eventData)
{
}

void MyApp::HandlePostRenderUpdate(StringHash eventType, VariantMap & eventData)
{
    
}

void MyApp::HandleEndFrame(StringHash eventType, VariantMap & eventData)
{
}

void MyApp::ShootObject()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    Node* boxNode = scene_->CreateChild("ShootBox");
    GetSubsystem<UI>()->GetRoot()->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));

    ScriptInstance* instance = boxNode->CreateComponent<ScriptInstance>();
    instance->CreateObject(cache->GetResource<ScriptFile>("Scripts/ShootBox.as"), "ShootBox");

    Quaternion rot = cameraNode->GetRotation();

    boxNode->SetPosition(cameraNode->GetWorldPosition() + rot * Vector3(0.0f, 0.1f, 2.0f) );
    boxNode->SetRotation(cameraNode->GetRotation());
    boxNode->SetScale( Vector3(0.1f, 0.1f, 2.0f) );

    StaticModel* boxObject = boxNode->CreateComponent<StaticModel>();
    boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    boxObject->SetMaterial(cache->GetResource<Material>("Materials/StoneEnvMapSmall.xml"));
    //boxObject->SetCastShadows(true);

    RigidBody* body = boxNode->CreateComponent<RigidBody>();
    body->SetMass(0.25f);
    body->SetFriction(0.75f);

    //Constant collision for fast objects
    body->SetCcdRadius(1.0f);
    body->SetCcdMotionThreshold(0.5);

    //body->SetTrigger(true);

    CollisionShape* shape = boxNode->CreateComponent<CollisionShape>();
    shape->SetBox(Vector3::ONE);

    const float OBJECT_VELOCITY = 100.0f;

    body->SetLinearVelocity(cameraNode->GetRotation() * Vector3(0.0f, 0.0f, 1.0f) * OBJECT_VELOCITY);
}

void MyApp::MovePlayer(float timeStep)
{
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input = GetSubsystem<Input>();
    if (character_)
    {
        // Clear previous controls
        character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

        character_->controls_.Set(CTRL_FORWARD, input->GetKeyDown('W'));
        character_->controls_.Set(CTRL_BACK, input->GetKeyDown('S'));
        character_->controls_.Set(CTRL_LEFT, input->GetKeyDown('A'));
        character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown('D'));

        character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));

        character_->controls_.yaw_ += (float)input->GetMouseMoveX() * YAW_SENSITIVITY;
        character_->controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;

        // Limit pitch
        character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);
        // Set rotation already here so that it's updated every rendering frame instead of every physics frame
        character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, up_));
    }
}

void MyApp::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input = GetSubsystem<Input>();

    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    if(flymode)
    {
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

        float movemult = 1;

        // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
        // Use the Translate() function (default local space) to move relative to the node's orientation.
        if (input->GetKeyDown('W'))
            cameraNode->Translate(Vector3::FORWARD * walkspeed * timeStep);
        if (input->GetKeyDown('S'))
            cameraNode->Translate(Vector3::BACK * walkspeed * timeStep);
        if (input->GetKeyDown('A'))
            cameraNode->Translate(Vector3::LEFT * walkspeed * timeStep);
        if (input->GetKeyDown('D'))
            cameraNode->Translate(Vector3::RIGHT * walkspeed * timeStep);
    }
}

void MyApp::HandleCollideStart(StringHash eventType, VariantMap& eventData)
{
    Node *src = static_cast<Node *>(eventData[PhysicsCollisionStart::P_NODEB].GetPtr());
    if(src->GetName() == "ShootBox")
    {
//        src->Remove();
    }
}

void MyApp::HandleConsoleCommand(StringHash eventType, VariantMap& eventData)
{
    using namespace ConsoleCommand;

    //Print(eventData[P_ID].GetString());
    //Print(GetTypeName());

    if (eventData[P_ID].GetString() == GetTypeName())
    {
        HandleInput(eventData[P_COMMAND].GetString());
    }
}

void MyApp::HandleInput(const String& input)
{
    Vector<String> ilist;

    String inputLower = input.ToLower().Trimmed();
    if (inputLower.Empty())
    {
        Print("Empty input given!");
        return;
    }

    ilist = inputLower.Split(' ');

    if(ilist[0] == "quit") { engine_->Exit(); return; }
    if(ilist[0] == "walkspeed")
    {
        if(ilist.Size() < 2) { Print("Usage: walkspeed <speed>"); return; }
        walkspeed = atof(ilist[1].CString());
        Print("Walk speed set to " + ilist[1]);
        return;
    }

    if(ilist[0] == "gravity")
    {
        if(ilist.Size() < 2) { Print("Usage: gravity <force>"); return; }

        PODVector<Component *> clist;
        scene_->GetComponents(clist, Urho3D::StringHash("RigidBody"), true);

        float g = atof(ilist[1].CString());
        //if(g > 0) { up_ = Vector3::DOWN; }
        //else { up_ = Vector3::UP; }

        pw_->SetGravity( Urho3D::Vector3(0.0, g, 0.0) );

        for(Urho3D::RandomAccessIterator<Component *> it = clist.Begin(); it != clist.End(); ++it)
        {
            RigidBody *rb = reinterpret_cast<RigidBody *>(*it);
            rb->Activate();
            rb->UpdateGravity();
        }

        Print("Gravity set to " + ilist[1]);
        return;
    }

    if(ilist[0] == "debughud")
    {
        if(ilist.Size() < 2) { Print("Usage: debughud 1|0|on|off"); return; }
        if(ilist[1] == "0" || ilist[1] == "off")
        {
            debughud->SetMode(DEBUGHUD_SHOW_NONE);
        }
        else
        {
            debughud->SetMode(DEBUGHUD_SHOW_ALL);
        }
        return;
    }

    Print("Unknown command: " + ilist[0]);
}

void MyApp::Print(const String& output)
{
    // Logging appears both in the engine console and stdout
    URHO3D_LOGRAW(output + "\n");
}

URHO3D_DEFINE_APPLICATION_MAIN(MyApp)
