#include "MaterialVulkan.h"
#include <windows.h>
#include <streambuf>
#include <sstream>
#include <istream>
#include <fstream>
#include <vector>
#include <set>
#include <assert.h>
#include <shaderc\shaderc.hpp>
#include <iostream>
#include "VulkanRenderer.h"
#include "../IA.h"

MaterialVulkan::MaterialVulkan(const std::string& name)
{
	isValid = false;

}


MaterialVulkan::~MaterialVulkan()
{
	vkDestroyDescriptorSetLayout(VulkanRenderer::device, descriptorSetLayout, nullptr);
	removeShader(ShaderType::VS);
	removeShader(ShaderType::PS);
}

void MaterialVulkan::setShader(const std::string & shaderFileName, ShaderType type)
{
	if (shaderFileNames.find(type) != shaderFileNames.end())
	{
		removeShader(type);
	}
	shaderFileNames[type] = shaderFileName;
}

void MaterialVulkan::removeShader(ShaderType type)
{
	if (shaderObjects[(int)type] != NULL)
	{
		vkDestroyShaderModule(VulkanRenderer::device, shaderObjects[(int)type], nullptr);
		shaderObjects[(int)type] = NULL;
		shaderStages[(int)type] = {};
	}
	
}

void MaterialVulkan::setDiffuse(Color c)
{
}

int MaterialVulkan::compileMaterial(std::string & errString)
{
	//Remove existing shaders
	removeShader(ShaderType::VS);
	removeShader(ShaderType::PS);

	// compile shaders
	std::string err;
	if (compileShader(ShaderType::VS, err) < 0) {
		errString = err;
		fprintf(stderr, "%s", errString.c_str());
		exit(-1);
	};
	if (compileShader(ShaderType::PS, err) < 0) {
		errString = err;
		fprintf(stderr, "%s", errString.c_str());
		exit(-1);
	};
	
	//link the shaders
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = shaderObjects[(int)ShaderType::VS];
	vertShaderStageInfo.pName = "main";
	shaderStages[(int)ShaderType::VS] = vertShaderStageInfo;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = shaderObjects[(int)ShaderType::PS];;
	fragShaderStageInfo.pName = "main";
	shaderStages[(int)ShaderType::PS] = fragShaderStageInfo;
	
	return 0;
}

void MaterialVulkan::addConstantBuffer(std::string name, unsigned int location)
{
}

void MaterialVulkan::updateConstantBuffer(const void * data, size_t size, unsigned int location)
{
}

int MaterialVulkan::enable()
{
	return 0;
}

void MaterialVulkan::disable()
{
}
VkPipelineShaderStageCreateInfo * MaterialVulkan::getShaderStages()
{
	return shaderStages;
}
VkDescriptorSetLayout * MaterialVulkan::getDescriptorSetLayout()
{
	return &descriptorSetLayout;
}
int MaterialVulkan::compileShader(ShaderType type, std::string & errString)
{
	// open the file and read it to a string "shaderText"
	std::ifstream shaderFile(shaderFileNames[type]);
	std::string shaderText;
	if (shaderFile.is_open()) {
		shaderText = std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
		shaderFile.close();
	}
	else {
		errString = "Cannot find file: " + shaderFileNames[type];
		return -1;
	}

	// make final string with shader source + defines + GLSL version + pragma shader type.
	// in theory this uses move semantics (compiler does it automagically)
	std::string expandedShader = expandShaderText(shaderText, type);


	shaderc_shader_kind shaderType;
	switch (type)
	{
	case ShaderType::VS:
		shaderType = shaderc_glsl_vertex_shader;
		break;
	case ShaderType::PS:
		shaderType = shaderc_glsl_fragment_shader;
		break;
	case ShaderType::GS:
		shaderType = shaderc_glsl_geometry_shader;
		break;
	case ShaderType::CS:
		shaderType = shaderc_glsl_compute_shader;
		break;
	default:
		shaderType = shaderc_glsl_infer_from_source;
		break;
	}

	shaderc::Compiler spirVCompiler;
	shaderc::CompileOptions options;
	shaderc::SpvCompilationResult result = spirVCompiler.CompileGlslToSpv(expandedShader, shaderType, shaderFileNames[type].c_str());
	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		errString = "Cannot compile shader " + shaderFileNames[type] + "\n error: " + result.GetErrorMessage();
		return -1;
	}

	std::vector<uint32_t> binaryShader(result.cbegin(), result.cend());
	//binaryShader.resize(binaryShader.size() + (4 - binaryShader.size() % 4)); //pad spir-v binary to be multiple of 4.
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = binaryShader.size() * sizeof(uint32_t);
	createInfo.pCode = binaryShader.data();
	
	VkShaderModule shaderModule;
	if (FAILED(vkCreateShaderModule(VulkanRenderer::device, &createInfo, nullptr, &shaderModule)))
	{
		errString = "Failed to create shader module for shader: " + shaderFileNames[type];
		return -1;
	}
	
	shaderObjects[(int)type] = shaderModule;
	
	createDescriptorSets();

	return 0;
}

void MaterialVulkan::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = POSITION;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	bindings.push_back(uboLayoutBinding);

	uboLayoutBinding.binding = NORMAL;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
	bindings.push_back(uboLayoutBinding);

	uboLayoutBinding.binding = TEXTCOORD;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
	bindings.push_back(uboLayoutBinding);

	uboLayoutBinding.binding = TRANSLATION;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings.push_back(uboLayoutBinding);

	uboLayoutBinding.binding = DIFFUSE_TINT;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
	bindings.push_back(uboLayoutBinding);

	//texture
	uboLayoutBinding.binding = DIFFUSE_SLOT;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
	bindings.push_back(uboLayoutBinding);

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();


	if (FAILED(vkCreateDescriptorSetLayout(VulkanRenderer::device, &layoutInfo, nullptr, &descriptorSetLayout)))
	{
		fprintf(stderr, "failed to create descriptor set layout!\n");
		exit(-1);
	}

}

std::string MaterialVulkan::expandShaderText(std::string & shaderText, ShaderType type)
{
	std::string result = "\n\n #version 450\n\0";
	for (auto define : shaderDefines[type])
	{
		result += define + "\n";
	}
	result += shaderText;

	return result;
}
