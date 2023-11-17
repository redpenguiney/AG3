#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include "texture.hpp"
#include <optional>

// A material is just a collection of textures.
// On the GPU, each object has a materialId that it uses to get color, normal, etc. textures.
// On the CPU, meshpools are sorted by their material to minimize texture bindings.
class Material {
    public:
    const unsigned int id;

    static std::shared_ptr<Material>& Get(const unsigned int id);
    static void Destroy(const unsigned int id);

    // makes all things be drawn with these textures
    void Use();

    // textureParams must at minimum contain a TextureCreateParams for color.
    // If possible, will not to create a new material, but simply add the requested textures to an existing compatible material.
    // Returns a pair of (textureZ, ptr to the created material)
    static std::pair<float, std::shared_ptr<Material>> New(const std::vector<TextureCreateParams>& textureParams);

    ~Material();
    
    private:
    Material(const std::shared_ptr<Texture>& colorMap, const std::shared_ptr<Texture>& normalMap, const std::shared_ptr<Texture>& specularMap);

    std::optional<Texture> color;
    std::optional<Texture> normal; 
    std::optional<Texture> specular;

    inline static std::unordered_map<unsigned int,  std::shared_ptr<Material>> MATERIALS;
};