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
    local c = Vec4f.new(0.5, 0.5, 0.5, 1.0)
    while true do
        print("iteration "..tostring(i))
        go.render.color = c
        c = c + Vec4f.new(math.random(-1, 1) * 0.01, math.random(-1, 1) * 0.01, math.random(-1, 1) * 0.01, 0.0)
        Wait(0.0)
        -- print("again?")
    end

    print("LUA WON!!!") 

    go:Destroy()
    print(transform.position)
    print(go.transform.position)
end

return test





-- error("woah, we should not have gotten here")

