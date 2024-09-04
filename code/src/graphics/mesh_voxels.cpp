#include "mesh.hpp"
#include "debug/assert.hpp"




// implementation for dual contouring to create mesh from terrain is in this file.

glm::vec3 GetNormalAtPoint(glm::vec3 pos, std::function<float(glm::vec3)> distanceFunction, float precision = 0.001) {
	return glm::normalize(glm::vec3 {
		(distanceFunction({pos.x + precision, pos.y, pos.z}) - distanceFunction({pos.x - precision, pos.y, pos.z})) / (precision * 2),
		(distanceFunction({pos.x, pos.y + precision, pos.z}) - distanceFunction({pos.x, pos.y - precision, pos.z})) / (precision * 2),
		(distanceFunction({pos.x, pos.y, pos.z + precision}) - distanceFunction({pos.x, pos.y, pos.z - precision})) / (precision * 2),
	});
}

std::optional<glm::vec3> FindBestVertex(
	glm::vec3 cellPos,
	float resolution,
	bool blocky,
	std::function<float(glm::vec3)> distanceFunction) 
{
	//if (blocky) {
		//return cellPos + (resolution * 0.5f);
	//}
	//else {

		// evaluate the distance function at each corner of the cell
		float distances[2][2][2];
		int insideCount = 0;
		for (unsigned int x = 0; x < 2; x++) {
			for (unsigned int y = 0; y < 2; y++) {
				for (unsigned int z = 0; z < 2; z++) {
					float distance = distanceFunction({ cellPos.x + resolution * x, cellPos.y + resolution * y, cellPos.z + resolution * z });
					distances[x][y][z] = distance;
					insideCount += distance <= 0;
				}
			}
		}
		if (insideCount == 0 || insideCount == 8) { // then voxel is either empty or full, nothing to render here
			return std::nullopt;
		}

		// for each edge between those corners, see if there's a sign change and if so store its position
		glm::vec3 signChanges[12];
		unsigned int nChanges = 0;

		for (unsigned int x = 0; x < 2; x++) {
			for (unsigned int y = 0; y < 2; y++) {
				float v0 = distances[x][y][0];
				float v1 = distances[x][y][1];
				if (v0 > 0 != v1 > 0) {
					// postfix gives nChanges correct valaue
					signChanges[nChanges++] = { cellPos.x + resolution * x, cellPos.y + resolution * y, cellPos.z + resolution * (-v0 / (v1 - v0)) };
				}
			}
		}

		for (unsigned int x = 0; x < 2; x++) {
			for (unsigned int z = 0; z < 2; z++) {
				float v0 = distances[x][0][z];
				float v1 = distances[x][1][z];
				if (v0 > 0 != v1 > 0) {
					// postfix gives nChanges correct valaue
					signChanges[nChanges++] = { cellPos.x + resolution * x, cellPos.y + resolution * (-v0 / (v1 - v0)), cellPos.z + resolution * z };
				}
			}
		}

		for (unsigned int y = 0; y < 2; y++) {
			for (unsigned int z = 0; z < 2; z++) {
				float v0 = distances[0][y][z];
				float v1 = distances[1][y][z];
				if (v0 > 0 != v1 > 0) {
					// postfix gives nChanges correct valaue
					signChanges[nChanges++] = { cellPos.x + resolution * (-v0 / (v1 - v0)), cellPos.y + resolution * y, cellPos.z + resolution * z};
				}
			}
		}

		if (nChanges <= 1) {
			return std::nullopt;
		}
		else {
			if (blocky) { return cellPos + (resolution * 0.5f); }

			glm::vec3 total = {0, 0, 0};
			for (unsigned int i = 0; i < nChanges; i++) {
				total += signChanges[i] - cellPos;
			}
			total /= float(nChanges);
			
			// clamp position (while preserving direction)
			//total -= total * resolution / glm::length(total - cellPos);
			//total = glm::min(glm::max(total, cellPos - resolution/2), cellPos + resolution/2);
			return total + cellPos;
		}


	//}
}

unsigned int IndexFromCell(glm::uvec3 xyz, glm::uvec3 dim) {
	return xyz.x * dim.y * dim.z + xyz.y * dim.z + xyz.z;
}

//std::optional<std::shared_ptr<Mesh>> Mesh::FromVoxels(
//	const MeshCreateParams& params, 
//	glm::vec3 p1,
//	glm::vec3 p2,
//	float resolution,
//	std::function<float(glm::vec3)> distanceFunction, 
//	std::optional<const TextureAtlas*> atlas, 
//	bool fixVertexCenters)
//{
//	
//
//	// create/return actual mesh object
//	unsigned int meshId = MeshGlobals::Get().LAST_MESH_ID; // (creating a mesh increments this)
//
//	auto realParams = params;
//	if (realParams.meshVertexFormat.has_value() == false) { realParams.meshVertexFormat.emplace(MeshVertexFormat::Default()); }
//
//	auto mesh = std::shared_ptr<Mesh>(new Mesh(vertices, indices, realParams));
//	MeshGlobals::Get().LOADED_MESHES[meshId] = mesh;
//	return mesh;
//}

DualContouringMeshProvider::DualContouringMeshProvider(const MeshCreateParams& params): MeshProvider(params)
{

}

std::pair<std::vector<float>, std::vector<unsigned int>> DualContouringMeshProvider::GetMesh() const
{
	// based on https://github.com/BorisTheBrave/mc-dc/blob/a165b326849d8814fb03c963ad33a9faf6cc6dea/dual_contour_3d.py
	// and now that it doesn't work, also https://github.com/emilk/Dual-Contouring/blob/master/src/vol/Contouring.cpp
	auto p1 = point1;
	auto p2 = point2 + resolution;

	Assert(!atlas.has_value() || atlas.value() != nullptr);

	auto format = meshParams.meshVertexFormat.value_or(MeshVertexFormat::Default());
	Assert(format.attributes.position.has_value() && format.attributes.position->nFloats == 3 && format.attributes.position->instanced == false);
	Assert(format.attributes.normal.has_value() && format.attributes.normal->nFloats == 3 && format.attributes.normal->instanced == false);
	auto fdim = (p2 - p1);
	Assert(std::fmodf(fdim.x, resolution) == 0);
	Assert(std::fmodf(fdim.y, resolution) == 0);
	Assert(std::fmodf(fdim.z, resolution) == 0);
	glm::uvec3 dim = glm::ceil((p2 - p1) / resolution);

	// find vertices
	std::vector<glm::vec3> vertexPositions;
	std::vector<GLuint> cellIndicesToVertexIndices; // key is IndexFromCell(), value is key for vertexPositions
	cellIndicesToVertexIndices.resize(dim.x * dim.y * dim.z);

	for (unsigned int cellX = 0; cellX < dim.x; cellX++) {
		for (unsigned int cellY = 0; cellY < dim.y; cellY++) {
			for (unsigned int cellZ = 0; cellZ < dim.z; cellZ++) {
				glm::vec3 worldPos = glm::vec3(cellX, cellY, cellZ) * resolution + p1;
				auto vert = FindBestVertex(worldPos, resolution, fixVertexCenters, distanceFunction);


				if (vert.has_value()) {
					if (cellX == 0) {
						//DebugLogInfo("P1 = ", glm::to_string(p1), " vPos = ", glm::to_string(*vert));
					}
					unsigned int i = IndexFromCell({ cellX, cellY, cellZ }, dim);
					cellIndicesToVertexIndices.at(i) = vertexPositions.size();
					vertexPositions.push_back(vert.value() - p1);
				}
			}
		}
	}

	// find indices
	std::vector<GLuint> indices;



	// if one cell's edge has positive distance (outside terrain) and another has negative distance (inside terrain), then a face of the boundary should be between those two faces.
	for (unsigned int cellX = 0; cellX < dim.x; cellX++) {
		for (unsigned int cellY = 0; cellY < dim.y; cellY++) {
			for (unsigned int cellZ = 0; cellZ < dim.z; cellZ++) {

				bool cellHasFaces = false;

				glm::vec3 worldPos = glm::vec3(cellX, cellY, cellZ) * resolution + p1;

				if (cellX > 0 && cellY > 0) {

					bool positive1 = distanceFunction({ worldPos.x, worldPos.y, worldPos.z }) > 0;
					bool positive2 = distanceFunction({ worldPos.x, worldPos.y, worldPos.z + resolution }) > 0;

					if (positive1 != positive2) { // then add face
						if (positive1 > positive2) { // then do opposite winding order (to fix backface culling)
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY - 1, cellZ }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY - 0, cellZ }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY - 1, cellZ }, dim)]);

							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY - 1, cellZ }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY - 0, cellZ }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY - 0, cellZ }, dim)]);
						}
						else {
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY - 1, cellZ }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY - 1, cellZ }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY - 0, cellZ }, dim)]);

							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY - 1, cellZ }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY - 0, cellZ }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY - 0, cellZ }, dim)]);
						}
					}
				}

				if (cellX > 0 && cellZ > 0) {

					bool positive1 = distanceFunction({ worldPos.x, worldPos.y, worldPos.z }) > 0;
					bool positive2 = distanceFunction({ worldPos.x, worldPos.y + resolution, worldPos.z }) > 0;

					if (positive1 != positive2) { // then add face
						if (positive1 > positive2) { // then do opposite winding order (to fix backface culling)
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY, cellZ - 1 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY, cellZ - 1 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY, cellZ - 0 }, dim)]);

							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY, cellZ - 1 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY, cellZ - 0 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY, cellZ - 0 }, dim)]);
						}
						else {
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY, cellZ - 1 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY, cellZ - 0 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY, cellZ - 1 }, dim)]);

							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY, cellZ - 1 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 1, cellY, cellZ - 0 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX - 0, cellY, cellZ - 0 }, dim)]);
						}
					}
				}

				if (cellY > 0 && cellZ > 0) {

					bool positive1 = distanceFunction({ worldPos.x, worldPos.y, worldPos.z }) > 0;
					bool positive2 = distanceFunction({ worldPos.x + resolution, worldPos.y, worldPos.z }) > 0;

					if (positive1 != positive2) { // then add face
						if (positive1 < positive2) { // then do opposite winding order (so backface culling works)
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 1, cellZ - 1 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 0, cellZ - 1 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 0, cellZ - 0 }, dim)]);

							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 1, cellZ - 0 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 1, cellZ - 1 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 0, cellZ - 0 }, dim)]);
						}
						else {
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 1, cellZ - 1 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 0, cellZ - 0 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 0, cellZ - 1 }, dim)]);

							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 1, cellZ - 0 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 0, cellZ - 0 }, dim)]);
							indices.push_back(cellIndicesToVertexIndices[IndexFromCell({ cellX, cellY - 1, cellZ - 1 }, dim)]);
						}
					}
				}

			}
		}
	}
	/*for (float x = p1.x; x <= p2.x; x += resolution) {
		for (float y = p1.y; y <= p2.y; y += resolution) {
			for (float z = p1.z; z <= p2.z; z += resolution) {



			}
		}
	}*/

	// write vertex data
	// TODO: UVs, tangents, colors
	std::vector<GLfloat> vertices;

	// the +2 is because, to ensure the mesh is normalized in a way that leaves its originalSize at p2 - p1, we gonna have two dummy vertices at p1 and p2 that aren't drawn.
	vertices.resize((vertexPositions.size() + 2) * format.GetNonInstancedVertexSize() / sizeof(GLfloat));

	Assert(indices.size() % 3 == 0);

	for (unsigned int posI = 0; posI < vertexPositions.size(); posI++) {

		vertices.at(format.attributes.position->offset / sizeof(GLfloat) + posI * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 0) = vertexPositions[posI].x;
		vertices.at(format.attributes.position->offset / sizeof(GLfloat) + posI * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 1) = vertexPositions[posI].y;
		vertices.at(format.attributes.position->offset / sizeof(GLfloat) + posI * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 2) = vertexPositions[posI].z;

		/*glm::vec3 normal = GetNormalAtPoint(vertexPositions[posI], distanceFunction);
		vertices.at(format.attributes.normal->offset / sizeof(GLfloat) + posI * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 0) = normal.x;
		vertices.at(format.attributes.normal->offset / sizeof(GLfloat) + posI * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 1) = normal.y;
		vertices.at(format.attributes.normal->offset / sizeof(GLfloat) + posI * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 2) = normal.z;*/
	}

	// Write normals; each vertex has a few faces attached to it with different normals, so we'll just add all those normals together and let the shader normalize it.
	for (unsigned int triangle = 0; triangle < indices.size() / 3; triangle++) {
		glm::vec3 triangleVerts[3];
		for (unsigned int i = 0; i < 3; i++) {
			unsigned int vertexIndex = indices.at(triangle * 3 + i);
			triangleVerts[i] = vertexPositions[vertexIndex];
		}

		glm::vec3 normal = glm::normalize(glm::cross(triangleVerts[2] - triangleVerts[0], triangleVerts[2] - triangleVerts[1]));
		
		for (unsigned int i = 0; i < 3; i++) {
			unsigned int vertexIndex = indices[triangle * 3 + i];
			vertices.at(format.attributes.normal->offset / sizeof(GLfloat) + vertexIndex * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 0) += normal.x;
			vertices.at(format.attributes.normal->offset / sizeof(GLfloat) + vertexIndex * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 1) += normal.y;
			vertices.at(format.attributes.normal->offset / sizeof(GLfloat) + vertexIndex * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 2) += normal.z;
			if (glm::length(glm::cross(triangleVerts[2] - triangleVerts[0], triangleVerts[2] - triangleVerts[1])) == 0) {
				DebugLogInfo("Verts = ", glm::to_string(triangleVerts[2]), ", ", glm::to_string(triangleVerts[1]), ", ", glm::to_string(triangleVerts[0]));
				DebugLogInfo("Adding normal ", glm::to_string(normal), " to vertex ", vertexIndex);
			}
			
		}
	}

	//if (vertexPositions.size() == 0 || indices.size() == 0) { return std::nullopt; }

	// set dummy vertices at corners
	vertices.at(format.attributes.position->offset / sizeof(GLfloat) + vertexPositions.size() * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 0) = 0;//p1.x;
	vertices.at(format.attributes.position->offset / sizeof(GLfloat) + vertexPositions.size() * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 1) = 0;//p1.y;
	vertices.at(format.attributes.position->offset / sizeof(GLfloat) + vertexPositions.size() * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 2) = 0;//p1.z;
	vertices.at(format.attributes.position->offset / sizeof(GLfloat) + (vertexPositions.size() + 1) * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 0) = p2.x - p1.x;
	vertices.at(format.attributes.position->offset / sizeof(GLfloat) + (vertexPositions.size() + 1) * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 1) = p2.y - p1.y;
	vertices.at(format.attributes.position->offset / sizeof(GLfloat) + (vertexPositions.size() + 1) * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + 2) = p2.z - p1.z;

	return std::pair(vertices, indices);
}