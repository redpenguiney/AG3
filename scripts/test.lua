    print("hola from lua!!!")
    -- print("Graphics engine is "..tostring(GE))

    -- print("Enum is " .. tostring(Enum));
    -- print("Enum.ComponentBitIndex.RenderNoFO = " .. tostring(Enum.ComponentBitIndex.Rigidbody))

    local makeMparams = MeshCreateParams.new()
    makeMparams.textureZ = -1.0
    makeMparams.opacity = 1
    makeMparams.expectedCount = 16384

    

    local m = Mesh.new("../models/rainbowcube.obj", makeMparams); 

    local brickMaterial, brickTextureZ = Material.new(Enum.TextureType.Texture2D,
        TextureCreateParams.new("../textures/ambientcg_bricks085/color.jpg", Enum.TextureUsage.ColorMap), 
        TextureCreateParams.new("../textures/ambientcg_bricks085/roughness.jpg", Enum.TextureUsage.SpecularMap), 
        TextureCreateParams.new("../textures/ambientcg_bricks085/normal_gl.jpg", Enum.TextureUsage.NormalMap)
        --TextureCreateParams {.texturePaths = {"../textures/ambientcg_bricks085/displacement.jpg"}, .format = Grayscale, .usage = DisplacementMap}
    )

    -- todo: you can put garbage values in here without consequences
    local params = GameObjectCreateParams.new({Enum.ComponentBitIndex.Transform, Enum.ComponentBitIndex.Render, Enum.ComponentBitIndex.Rigidbody, Enum.ComponentBitIndex.Collider})
    params.meshId = m.id
    params.materialId = brickMaterial.id

    local go = GameObject.new(params)

    
    go.render.textureZ = brickTextureZ
    

    local vec3 = Vec3d.new(0, 2, 3)
    -- print(go.transform);
    local transform = go.transform;
    -- print(transform);
    -- print(transform.position)
    

    vec3 = vec3 * 1.0
    -- print(vec3.z)
    transform.position = vec3 * vec3
    transform.position = vec3 * 1.0
    transform.rotation = Quat.new(Vec3f.new(0.0, 3.14/8.0, 0.0))
    -- coroutine.yield()
    -- print("here we go");

    

    local c = Vec4f.new(0.5, 0.5, 0.5, 1.0)
    while true do
        
        -- go.render.color = c
        -- c = c + Vec4f.new(math.random(-1, 1) * 0.01, math.random(-1, 1) * 0.01, math.random(-1, 1) * 0.01, 0.0)
        
        Wait(0.0)
        -- print("cool")
        -- print("again?")
    end

    
    
    -- print("LUA WON!!!") 

    -- go:Destroy()
    -- print(transform.position)
    -- print(go.transform.position)




-- error("woah, we should not have gotten here")

