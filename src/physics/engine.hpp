// it's a physics engine, obviously.
class PhysicsEngine {
    public:
    PhysicsEngine(PhysicsEngine const&) = delete; // no copying
    PhysicsEngine& operator=(PhysicsEngine const&) = delete; // no assigning

    static PhysicsEngine& Get();
    

    // Moves the physics simulation forward by timestep.
    void Step(const float timestep);

    private:

    PhysicsEngine();

    ~PhysicsEngine();
};