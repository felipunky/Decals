#ifndef _OPEN_GLTF_H_
#define _OPEN_GLTF_H_

#include <string>
#include <iostream>

namespace GLTF
{
    template <typename Type>
	bool GetAttributes(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<Type>& attributes, const std::string& name)
	{
		auto iter = primitive.attributes.find(name);
		if (iter == primitive.attributes.end())
			return false;

		//! Retrieving the data of the attributes
		const auto& accessor = model.accessors[iter->second];
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		const auto& buffer = model.buffers[bufferView.buffer];
		const Type* bufData = reinterpret_cast<const Type*>(&(buffer.data[accessor.byteOffset + bufferView.byteOffset]));
		const auto& numElements = accessor.count;

		//! Supporting KHR_mesh_quantization
		assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
		{
			if (bufferView.byteStride == 0)
			{
				attributes.insert(attributes.end(), bufData, bufData + numElements);
			}
			else
			{
				auto bufferByte = reinterpret_cast<const unsigned char*>(bufData);
				for (size_t i = 0; i < numElements; ++i)
				{
					attributes.push_back(*reinterpret_cast<const Type*>(bufferByte));
					bufferByte += bufferView.byteStride;
				}
			}
		}
		else //! The component is smaller than flaot and need to be covnerted (quantized)
		{
			//! vec3 or vec4
			int numComponents = accessor.type;
			//! unsigned byte or unsigned short.
			int strideComponent = accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ? 1 : 2;

			size_t byteStride = bufferView.byteStride > 0 ? bufferView.byteStride : numComponents * strideComponent;
			auto bufferByte = reinterpret_cast<const unsigned char*>(bufData);
			for (size_t i = 0; i < numElements; ++i)
			{
				Type vecValue;
				
				auto bufferByteData = bufferByte;
				for (int c = 0; c < numComponents; ++c)
				{
					float value = *reinterpret_cast<const float*>(bufferByteData);
					switch (accessor.componentType)
					{
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						vecValue[c] = value / std::numeric_limits<unsigned short>::max();
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						vecValue[c] = value / std::numeric_limits<unsigned char>::max();
						break;
					case TINYGLTF_COMPONENT_TYPE_SHORT:
						vecValue[c] = std::max(value / std::numeric_limits<short>::max(), -1.0f);
						break;
					case TINYGLTF_COMPONENT_TYPE_BYTE:
						vecValue[c] = std::max(value / std::numeric_limits<char>::max(), -1.0f);
						break;
					default:
						std::cerr << "Unknown attributes component type : " << accessor.componentType << " is not supported" << std::endl;
						return false;
					}
					bufferByteData += strideComponent;
				}
				bufferByte += byteStride;
				attributes.push_back(vecValue);
			}
		}

		return true;
	}

	bool loadModel(tinygltf::Model &model, const std::string filename)
	{
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		std::string extension = tinygltf::GetFilePathExtension(filename);

		bool res = false;

		if (extension.compare("glb") == 0)
		{
			std::cout << "Reading binary glTF" << std::endl;
			res = loader.LoadBinaryFromFile(&model, &err, &warn, filename.c_str());
		}
		else 
		{
			std::cout << "Reading ASCII glTF" << std::endl;
			res = loader.LoadASCIIFromFile(&model, &err, &warn, filename.c_str());
		}
		if (!warn.empty()) 
		{
			std::cout << "WARN: " << warn << std::endl;
		}

		if (!err.empty()) 
		{
			std::cout << "ERR: " << err << std::endl;
		}

		if (!res)
		{
			std::cout << "Failed to load glTF: " << filename << std::endl;
		}
#ifdef OPTIMIZE
#else
		else
		{
			std::cout << "Loaded glTF: " << filename << std::endl;
		}
#endif
		return res;
	}

	bool GetGLTFModel(tinygltf::Model* model, std::string& err, std::string& warn, const std::vector<unsigned char>& data)
	{
		std::string basedir = "";

		tinygltf::TinyGLTF t;

		bool result = t.LoadASCIIFromString(model, &err, &warn, reinterpret_cast<const char *>(&data.at(0)), 
										    static_cast<unsigned int>(data.size()), basedir, tinygltf::REQUIRE_VERSION);
		return result;
	}

	bool GetGLBModel(tinygltf::Model* model, std::string& err, std::string& warn, const std::vector<unsigned char>& data)
	{
		std::string basedir = "";

		tinygltf::TinyGLTF t;

		bool result = t.LoadBinaryFromMemory(model, &err, &warn, &data.at(0), 
										    static_cast<unsigned int>(data.size()), basedir);
		return result;
	}
}
#endif