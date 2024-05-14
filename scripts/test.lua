local test = function() 
    print("hola from lua!!!")
    print("Graphics engine is "..tostring(GE))

    print("Enum is " .. tostring(Enum));
    print("Enum.ComponentBitIndex.RenderNoFO = " .. tostring(Enum.ComponentBitIndex.Rigidbody))


    -- todo: you can put garbage values in here without consequences
    local params = GameObjectCreateParams.new({Enum.ComponentBitIndex.Transform, Enum.ComponentBitIndex.Render, Enum.ComponentBitIndex.Rigidbody})
    params.meshId = 3

    local go = GameObject.new(params)

    local vec3 = Vec3d.new(0, 2, 3)
    print(go.transform);
    local transform = go.transform;
    print(transform);
    print(transform.position)
    

    vec3 = vec3 * 1.0
    print(vec3.z)
    transform.position = vec3 * vec3
    transform.position = vec3 * 1.0
    transform.rotation = Quat.new(Vec3f.new(0.0, 3.14/8.0, 0.0))
    -- coroutine.yield()
    print("here we go");
    for i = 0, 10 do
        print("iteration "..tostring(i))
        Wait(0.2)
        -- print("again?")
    end

    print("LUA WON!!!") 

    go:Destroy()
    print(transform.position)
    print(go.transform.position)
end

return test





-- error("woah, we should not have gotten here")

