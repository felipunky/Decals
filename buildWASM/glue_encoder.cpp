
#include <emscripten.h>
#include <stdlib.h>

EM_JS_DEPS(webidl_binder, "$intArrayFromString,$UTF8ToString,$alignMemory");

extern "C" {

// Define custom allocator functions that we can force export using
// EMSCRIPTEN_KEEPALIVE.  This avoids all webidl users having to add
// malloc/free to -sEXPORTED_FUNCTIONS.
EMSCRIPTEN_KEEPALIVE void webidl_free(void* p) { free(p); }
EMSCRIPTEN_KEEPALIVE void* webidl_malloc(size_t len) { return malloc(len); }


// Interface: VoidPtr


void EMSCRIPTEN_KEEPALIVE emscripten_bind_VoidPtr___destroy___0(void** self) {
  delete self;
}

// Interface: GeometryAttribute


draco::GeometryAttribute* EMSCRIPTEN_KEEPALIVE emscripten_bind_GeometryAttribute_GeometryAttribute_0() {
  return new draco::GeometryAttribute();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_GeometryAttribute___destroy___0(draco::GeometryAttribute* self) {
  delete self;
}

// Interface: PointAttribute


draco::PointAttribute* EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute_PointAttribute_0() {
  return new draco::PointAttribute();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute_size_0(draco::PointAttribute* self) {
  return self->size();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute_attribute_type_0(draco::PointAttribute* self) {
  return self->attribute_type();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute_data_type_0(draco::PointAttribute* self) {
  return self->data_type();
}

char EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute_num_components_0(draco::PointAttribute* self) {
  return self->num_components();
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute_normalized_0(draco::PointAttribute* self) {
  return self->normalized();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute_byte_stride_0(draco::PointAttribute* self) {
  return self->byte_stride();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute_byte_offset_0(draco::PointAttribute* self) {
  return self->byte_offset();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute_unique_id_0(draco::PointAttribute* self) {
  return self->unique_id();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute___destroy___0(draco::PointAttribute* self) {
  delete self;
}

// Interface: PointCloud


draco::PointCloud* EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloud_PointCloud_0() {
  return new draco::PointCloud();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloud_num_attributes_0(draco::PointCloud* self) {
  return self->num_attributes();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloud_num_points_0(draco::PointCloud* self) {
  return self->num_points();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloud___destroy___0(draco::PointCloud* self) {
  delete self;
}

// Interface: Mesh


draco::Mesh* EMSCRIPTEN_KEEPALIVE emscripten_bind_Mesh_Mesh_0() {
  return new draco::Mesh();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Mesh_num_faces_0(draco::Mesh* self) {
  return self->num_faces();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Mesh_num_attributes_0(draco::Mesh* self) {
  return self->num_attributes();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Mesh_num_points_0(draco::Mesh* self) {
  return self->num_points();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Mesh_set_num_points_1(draco::Mesh* self, int num_points) {
  self->set_num_points(num_points);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Mesh___destroy___0(draco::Mesh* self) {
  delete self;
}

// Interface: Metadata


draco::Metadata* EMSCRIPTEN_KEEPALIVE emscripten_bind_Metadata_Metadata_0() {
  return new draco::Metadata();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Metadata___destroy___0(draco::Metadata* self) {
  delete self;
}

// Interface: DracoInt8Array


DracoInt8Array* EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt8Array_DracoInt8Array_0() {
  return new DracoInt8Array();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt8Array_GetValue_1(DracoInt8Array* self, int index) {
  return self->GetValue(index);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt8Array_size_0(DracoInt8Array* self) {
  return self->size();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt8Array___destroy___0(DracoInt8Array* self) {
  delete self;
}

// Interface: MetadataBuilder


MetadataBuilder* EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataBuilder_MetadataBuilder_0() {
  return new MetadataBuilder();
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataBuilder_AddStringEntry_3(MetadataBuilder* self, draco::Metadata* metadata, const char* entry_name, const char* entry_value) {
  return self->AddStringEntry(metadata, entry_name, entry_value);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataBuilder_AddIntEntry_3(MetadataBuilder* self, draco::Metadata* metadata, const char* entry_name, int entry_value) {
  return self->AddIntEntry(metadata, entry_name, entry_value);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataBuilder_AddIntEntryArray_4(MetadataBuilder* self, draco::Metadata* metadata, const char* entry_name, const int* att_values, int num_values) {
  return self->AddIntEntryArray(metadata, entry_name, att_values, num_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataBuilder_AddDoubleEntry_3(MetadataBuilder* self, draco::Metadata* metadata, const char* entry_name, double entry_value) {
  return self->AddDoubleEntry(metadata, entry_name, entry_value);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataBuilder___destroy___0(MetadataBuilder* self) {
  delete self;
}

// Interface: PointCloudBuilder


PointCloudBuilder* EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_PointCloudBuilder_0() {
  return new PointCloudBuilder();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_AddFloatAttribute_5(PointCloudBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const float* att_values) {
  return self->AddFloatAttribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_AddInt8Attribute_5(PointCloudBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const char* att_values) {
  return self->AddInt8Attribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_AddUInt8Attribute_5(PointCloudBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const unsigned char* att_values) {
  return self->AddUInt8Attribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_AddInt16Attribute_5(PointCloudBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const short* att_values) {
  return self->AddInt16Attribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_AddUInt16Attribute_5(PointCloudBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const unsigned short* att_values) {
  return self->AddUInt16Attribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_AddInt32Attribute_5(PointCloudBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const int* att_values) {
  return self->AddInt32Attribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_AddUInt32Attribute_5(PointCloudBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const unsigned int* att_values) {
  return self->AddUInt32Attribute(pc, type, num_vertices, num_components, att_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_AddMetadata_2(PointCloudBuilder* self, draco::PointCloud* pc, const draco::Metadata* metadata) {
  return self->AddMetadata(pc, metadata);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_SetMetadataForAttribute_3(PointCloudBuilder* self, draco::PointCloud* pc, int attribute_id, const draco::Metadata* metadata) {
  return self->SetMetadataForAttribute(pc, attribute_id, metadata);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder_SetNormalizedFlagForAttribute_3(PointCloudBuilder* self, draco::PointCloud* pc, int attribute_id, bool normalized) {
  return self->SetNormalizedFlagForAttribute(pc, attribute_id, normalized);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_PointCloudBuilder___destroy___0(PointCloudBuilder* self) {
  delete self;
}

// Interface: MeshBuilder


MeshBuilder* EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_MeshBuilder_0() {
  return new MeshBuilder();
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddFacesToMesh_3(MeshBuilder* self, draco::Mesh* mesh, int num_faces, const int* faces) {
  return self->AddFacesToMesh(mesh, num_faces, faces);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddFloatAttributeToMesh_5(MeshBuilder* self, draco::Mesh* mesh, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const float* att_values) {
  return self->AddFloatAttributeToMesh(mesh, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddInt32AttributeToMesh_5(MeshBuilder* self, draco::Mesh* mesh, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const int* att_values) {
  return self->AddInt32AttributeToMesh(mesh, type, num_vertices, num_components, att_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddMetadataToMesh_2(MeshBuilder* self, draco::Mesh* mesh, const draco::Metadata* metadata) {
  return self->AddMetadataToMesh(mesh, metadata);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddFloatAttribute_5(MeshBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const float* att_values) {
  return self->AddFloatAttribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddInt8Attribute_5(MeshBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const char* att_values) {
  return self->AddInt8Attribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddUInt8Attribute_5(MeshBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const unsigned char* att_values) {
  return self->AddUInt8Attribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddInt16Attribute_5(MeshBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const short* att_values) {
  return self->AddInt16Attribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddUInt16Attribute_5(MeshBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const unsigned short* att_values) {
  return self->AddUInt16Attribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddInt32Attribute_5(MeshBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const int* att_values) {
  return self->AddInt32Attribute(pc, type, num_vertices, num_components, att_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddUInt32Attribute_5(MeshBuilder* self, draco::PointCloud* pc, draco_GeometryAttribute_Type type, int num_vertices, int num_components, const unsigned int* att_values) {
  return self->AddUInt32Attribute(pc, type, num_vertices, num_components, att_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_AddMetadata_2(MeshBuilder* self, draco::PointCloud* pc, const draco::Metadata* metadata) {
  return self->AddMetadata(pc, metadata);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_SetMetadataForAttribute_3(MeshBuilder* self, draco::PointCloud* pc, int attribute_id, const draco::Metadata* metadata) {
  return self->SetMetadataForAttribute(pc, attribute_id, metadata);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder_SetNormalizedFlagForAttribute_3(MeshBuilder* self, draco::PointCloud* pc, int attribute_id, bool normalized) {
  return self->SetNormalizedFlagForAttribute(pc, attribute_id, normalized);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_MeshBuilder___destroy___0(MeshBuilder* self) {
  delete self;
}

// Interface: Encoder


Encoder* EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder_Encoder_0() {
  return new Encoder();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder_SetEncodingMethod_1(Encoder* self, int method) {
  self->SetEncodingMethod(method);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder_SetAttributeQuantization_2(Encoder* self, draco_GeometryAttribute_Type type, int quantization_bits) {
  self->SetAttributeQuantization(type, quantization_bits);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder_SetAttributeExplicitQuantization_5(Encoder* self, draco_GeometryAttribute_Type type, int quantization_bits, int num_components, const float* origin, float range) {
  self->SetAttributeExplicitQuantization(type, quantization_bits, num_components, origin, range);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder_SetSpeedOptions_2(Encoder* self, int encoding_speed, int decoding_speed) {
  self->SetSpeedOptions(encoding_speed, decoding_speed);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder_SetTrackEncodedProperties_1(Encoder* self, bool flag) {
  self->SetTrackEncodedProperties(flag);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder_EncodeMeshToDracoBuffer_2(Encoder* self, draco::Mesh* mesh, DracoInt8Array* encoded_data) {
  return self->EncodeMeshToDracoBuffer(mesh, encoded_data);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder_EncodePointCloudToDracoBuffer_3(Encoder* self, draco::PointCloud* pc, bool deduplicate_values, DracoInt8Array* encoded_data) {
  return self->EncodePointCloudToDracoBuffer(pc, deduplicate_values, encoded_data);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder_GetNumberOfEncodedPoints_0(Encoder* self) {
  return self->GetNumberOfEncodedPoints();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder_GetNumberOfEncodedFaces_0(Encoder* self) {
  return self->GetNumberOfEncodedFaces();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Encoder___destroy___0(Encoder* self) {
  delete self;
}

// Interface: ExpertEncoder


ExpertEncoder* EMSCRIPTEN_KEEPALIVE emscripten_bind_ExpertEncoder_ExpertEncoder_1(draco::PointCloud* pc) {
  return new ExpertEncoder(pc);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_ExpertEncoder_SetEncodingMethod_1(ExpertEncoder* self, int method) {
  self->SetEncodingMethod(method);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_ExpertEncoder_SetAttributeQuantization_2(ExpertEncoder* self, int att_id, int quantization_bits) {
  self->SetAttributeQuantization(att_id, quantization_bits);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_ExpertEncoder_SetAttributeExplicitQuantization_5(ExpertEncoder* self, int att_id, int quantization_bits, int num_components, const float* origin, float range) {
  self->SetAttributeExplicitQuantization(att_id, quantization_bits, num_components, origin, range);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_ExpertEncoder_SetSpeedOptions_2(ExpertEncoder* self, int encoding_speed, int decoding_speed) {
  self->SetSpeedOptions(encoding_speed, decoding_speed);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_ExpertEncoder_SetTrackEncodedProperties_1(ExpertEncoder* self, bool flag) {
  self->SetTrackEncodedProperties(flag);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_ExpertEncoder_EncodeToDracoBuffer_2(ExpertEncoder* self, bool deduplicate_values, DracoInt8Array* encoded_data) {
  return self->EncodeToDracoBuffer(deduplicate_values, encoded_data);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_ExpertEncoder_GetNumberOfEncodedPoints_0(ExpertEncoder* self) {
  return self->GetNumberOfEncodedPoints();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_ExpertEncoder_GetNumberOfEncodedFaces_0(ExpertEncoder* self) {
  return self->GetNumberOfEncodedFaces();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_ExpertEncoder___destroy___0(ExpertEncoder* self) {
  delete self;
}

// $draco_GeometryAttribute_Type
draco_GeometryAttribute_Type EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_GeometryAttribute_Type_INVALID() {
  return draco_GeometryAttribute::INVALID;
}
draco_GeometryAttribute_Type EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_GeometryAttribute_Type_POSITION() {
  return draco_GeometryAttribute::POSITION;
}
draco_GeometryAttribute_Type EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_GeometryAttribute_Type_NORMAL() {
  return draco_GeometryAttribute::NORMAL;
}
draco_GeometryAttribute_Type EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_GeometryAttribute_Type_COLOR() {
  return draco_GeometryAttribute::COLOR;
}
draco_GeometryAttribute_Type EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_GeometryAttribute_Type_TEX_COORD() {
  return draco_GeometryAttribute::TEX_COORD;
}
draco_GeometryAttribute_Type EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_GeometryAttribute_Type_GENERIC() {
  return draco_GeometryAttribute::GENERIC;
}

// $draco_EncodedGeometryType
draco_EncodedGeometryType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_EncodedGeometryType_INVALID_GEOMETRY_TYPE() {
  return draco::INVALID_GEOMETRY_TYPE;
}
draco_EncodedGeometryType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_EncodedGeometryType_POINT_CLOUD() {
  return draco::POINT_CLOUD;
}
draco_EncodedGeometryType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_EncodedGeometryType_TRIANGULAR_MESH() {
  return draco::TRIANGULAR_MESH;
}

// $draco_MeshEncoderMethod
draco_MeshEncoderMethod EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_MeshEncoderMethod_MESH_SEQUENTIAL_ENCODING() {
  return draco::MESH_SEQUENTIAL_ENCODING;
}
draco_MeshEncoderMethod EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_MeshEncoderMethod_MESH_EDGEBREAKER_ENCODING() {
  return draco::MESH_EDGEBREAKER_ENCODING;
}

}

