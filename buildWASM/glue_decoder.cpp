
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

// Interface: DecoderBuffer


draco::DecoderBuffer* EMSCRIPTEN_KEEPALIVE emscripten_bind_DecoderBuffer_DecoderBuffer_0() {
  return new draco::DecoderBuffer();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_DecoderBuffer_Init_2(draco::DecoderBuffer* self, const char* data, unsigned int data_size) {
  self->Init(data, data_size);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_DecoderBuffer___destroy___0(draco::DecoderBuffer* self) {
  delete self;
}

// Interface: AttributeTransformData


draco::AttributeTransformData* EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeTransformData_AttributeTransformData_0() {
  return new draco::AttributeTransformData();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeTransformData_transform_type_0(draco::AttributeTransformData* self) {
  return self->transform_type();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeTransformData___destroy___0(draco::AttributeTransformData* self) {
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

const draco::AttributeTransformData* EMSCRIPTEN_KEEPALIVE emscripten_bind_PointAttribute_GetAttributeTransformData_0(draco::PointAttribute* self) {
  return self->GetAttributeTransformData();
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

// Interface: AttributeQuantizationTransform


draco::AttributeQuantizationTransform* EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeQuantizationTransform_AttributeQuantizationTransform_0() {
  return new draco::AttributeQuantizationTransform();
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeQuantizationTransform_InitFromAttribute_1(draco::AttributeQuantizationTransform* self, const draco::PointAttribute* att) {
  return self->InitFromAttribute(*att);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeQuantizationTransform_quantization_bits_0(draco::AttributeQuantizationTransform* self) {
  return self->quantization_bits();
}

float EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeQuantizationTransform_min_value_1(draco::AttributeQuantizationTransform* self, int axis) {
  return self->min_value(axis);
}

float EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeQuantizationTransform_range_0(draco::AttributeQuantizationTransform* self) {
  return self->range();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeQuantizationTransform___destroy___0(draco::AttributeQuantizationTransform* self) {
  delete self;
}

// Interface: AttributeOctahedronTransform


draco::AttributeOctahedronTransform* EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeOctahedronTransform_AttributeOctahedronTransform_0() {
  return new draco::AttributeOctahedronTransform();
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeOctahedronTransform_InitFromAttribute_1(draco::AttributeOctahedronTransform* self, const draco::PointAttribute* att) {
  return self->InitFromAttribute(*att);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeOctahedronTransform_quantization_bits_0(draco::AttributeOctahedronTransform* self) {
  return self->quantization_bits();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_AttributeOctahedronTransform___destroy___0(draco::AttributeOctahedronTransform* self) {
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

// Interface: Status


draco_StatusCode EMSCRIPTEN_KEEPALIVE emscripten_bind_Status_code_0(draco::Status* self) {
  return self->code();
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Status_ok_0(draco::Status* self) {
  return self->ok();
}

const char* EMSCRIPTEN_KEEPALIVE emscripten_bind_Status_error_msg_0(draco::Status* self) {
  return self->error_msg();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Status___destroy___0(draco::Status* self) {
  delete self;
}

// Interface: DracoFloat32Array


DracoFloat32Array* EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoFloat32Array_DracoFloat32Array_0() {
  return new DracoFloat32Array();
}

float EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoFloat32Array_GetValue_1(DracoFloat32Array* self, int index) {
  return self->GetValue(index);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoFloat32Array_size_0(DracoFloat32Array* self) {
  return self->size();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoFloat32Array___destroy___0(DracoFloat32Array* self) {
  delete self;
}

// Interface: DracoInt8Array


DracoInt8Array* EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt8Array_DracoInt8Array_0() {
  return new DracoInt8Array();
}

char EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt8Array_GetValue_1(DracoInt8Array* self, int index) {
  return self->GetValue(index);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt8Array_size_0(DracoInt8Array* self) {
  return self->size();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt8Array___destroy___0(DracoInt8Array* self) {
  delete self;
}

// Interface: DracoUInt8Array


DracoUInt8Array* EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt8Array_DracoUInt8Array_0() {
  return new DracoUInt8Array();
}

unsigned char EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt8Array_GetValue_1(DracoUInt8Array* self, int index) {
  return self->GetValue(index);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt8Array_size_0(DracoUInt8Array* self) {
  return self->size();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt8Array___destroy___0(DracoUInt8Array* self) {
  delete self;
}

// Interface: DracoInt16Array


DracoInt16Array* EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt16Array_DracoInt16Array_0() {
  return new DracoInt16Array();
}

short EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt16Array_GetValue_1(DracoInt16Array* self, int index) {
  return self->GetValue(index);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt16Array_size_0(DracoInt16Array* self) {
  return self->size();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt16Array___destroy___0(DracoInt16Array* self) {
  delete self;
}

// Interface: DracoUInt16Array


DracoUInt16Array* EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt16Array_DracoUInt16Array_0() {
  return new DracoUInt16Array();
}

unsigned short EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt16Array_GetValue_1(DracoUInt16Array* self, int index) {
  return self->GetValue(index);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt16Array_size_0(DracoUInt16Array* self) {
  return self->size();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt16Array___destroy___0(DracoUInt16Array* self) {
  delete self;
}

// Interface: DracoInt32Array


DracoInt32Array* EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt32Array_DracoInt32Array_0() {
  return new DracoInt32Array();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt32Array_GetValue_1(DracoInt32Array* self, int index) {
  return self->GetValue(index);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt32Array_size_0(DracoInt32Array* self) {
  return self->size();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoInt32Array___destroy___0(DracoInt32Array* self) {
  delete self;
}

// Interface: DracoUInt32Array


DracoUInt32Array* EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt32Array_DracoUInt32Array_0() {
  return new DracoUInt32Array();
}

unsigned int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt32Array_GetValue_1(DracoUInt32Array* self, int index) {
  return self->GetValue(index);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt32Array_size_0(DracoUInt32Array* self) {
  return self->size();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_DracoUInt32Array___destroy___0(DracoUInt32Array* self) {
  delete self;
}

// Interface: MetadataQuerier


MetadataQuerier* EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataQuerier_MetadataQuerier_0() {
  return new MetadataQuerier();
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataQuerier_HasEntry_2(MetadataQuerier* self, const draco::Metadata* metadata, const char* entry_name) {
  return self->HasEntry(*metadata, entry_name);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataQuerier_GetIntEntry_2(MetadataQuerier* self, const draco::Metadata* metadata, const char* entry_name) {
  return self->GetIntEntry(*metadata, entry_name);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataQuerier_GetIntEntryArray_3(MetadataQuerier* self, const draco::Metadata* metadata, const char* entry_name, DracoInt32Array* out_values) {
  self->GetIntEntryArray(*metadata, entry_name, out_values);
}

double EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataQuerier_GetDoubleEntry_2(MetadataQuerier* self, const draco::Metadata* metadata, const char* entry_name) {
  return self->GetDoubleEntry(*metadata, entry_name);
}

const char* EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataQuerier_GetStringEntry_2(MetadataQuerier* self, const draco::Metadata* metadata, const char* entry_name) {
  return self->GetStringEntry(*metadata, entry_name);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataQuerier_NumEntries_1(MetadataQuerier* self, const draco::Metadata* metadata) {
  return self->NumEntries(*metadata);
}

const char* EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataQuerier_GetEntryName_2(MetadataQuerier* self, const draco::Metadata* metadata, int entry_id) {
  return self->GetEntryName(*metadata, entry_id);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_MetadataQuerier___destroy___0(MetadataQuerier* self) {
  delete self;
}

// Interface: Decoder


Decoder* EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_Decoder_0() {
  return new Decoder();
}

const draco::Status* EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_DecodeArrayToPointCloud_3(Decoder* self, const char* data, unsigned int data_size, draco::PointCloud* out_point_cloud) {
  return self->DecodeArrayToPointCloud(data, data_size, out_point_cloud);
}

const draco::Status* EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_DecodeArrayToMesh_3(Decoder* self, const char* data, unsigned int data_size, draco::Mesh* out_mesh) {
  return self->DecodeArrayToMesh(data, data_size, out_mesh);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeId_2(Decoder* self, const draco::PointCloud* pc, draco_GeometryAttribute_Type type) {
  return self->GetAttributeId(*pc, type);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeIdByName_2(Decoder* self, const draco::PointCloud* pc, const char* name) {
  return self->GetAttributeIdByName(*pc, name);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeIdByMetadataEntry_3(Decoder* self, const draco::PointCloud* pc, const char* name, const char* value) {
  return self->GetAttributeIdByMetadataEntry(*pc, name, value);
}

const draco::PointAttribute* EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttribute_2(Decoder* self, const draco::PointCloud* pc, int att_id) {
  return self->GetAttribute(*pc, att_id);
}

const draco::PointAttribute* EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeByUniqueId_2(Decoder* self, const draco::PointCloud* pc, int unique_id) {
  return self->GetAttributeByUniqueId(*pc, unique_id);
}

const draco::Metadata* EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetMetadata_1(Decoder* self, const draco::PointCloud* pc) {
  return self->GetMetadata(*pc);
}

const draco::Metadata* EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeMetadata_2(Decoder* self, const draco::PointCloud* pc, int att_id) {
  return self->GetAttributeMetadata(*pc, att_id);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetFaceFromMesh_3(Decoder* self, const draco::Mesh* m, int face_id, DracoInt32Array* out_values) {
  return self->GetFaceFromMesh(*m, face_id, out_values);
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetTriangleStripsFromMesh_2(Decoder* self, const draco::Mesh* m, DracoInt32Array* strip_values) {
  return self->GetTriangleStripsFromMesh(*m, strip_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetTrianglesUInt16Array_3(Decoder* self, const draco::Mesh* m, int out_size, void* out_values) {
  return self->GetTrianglesUInt16Array(*m, out_size, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetTrianglesUInt32Array_3(Decoder* self, const draco::Mesh* m, int out_size, void* out_values) {
  return self->GetTrianglesUInt32Array(*m, out_size, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeFloat_3(Decoder* self, const draco::PointAttribute* pa, int att_index, DracoFloat32Array* out_values) {
  return self->GetAttributeFloat(*pa, att_index, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeFloatForAllPoints_3(Decoder* self, const draco::PointCloud* pc, const draco::PointAttribute* pa, DracoFloat32Array* out_values) {
  return self->GetAttributeFloatForAllPoints(*pc, *pa, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeIntForAllPoints_3(Decoder* self, const draco::PointCloud* pc, const draco::PointAttribute* pa, DracoInt32Array* out_values) {
  return self->GetAttributeIntForAllPoints(*pc, *pa, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeInt8ForAllPoints_3(Decoder* self, const draco::PointCloud* pc, const draco::PointAttribute* pa, DracoInt8Array* out_values) {
  return self->GetAttributeInt8ForAllPoints(*pc, *pa, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeUInt8ForAllPoints_3(Decoder* self, const draco::PointCloud* pc, const draco::PointAttribute* pa, DracoUInt8Array* out_values) {
  return self->GetAttributeUInt8ForAllPoints(*pc, *pa, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeInt16ForAllPoints_3(Decoder* self, const draco::PointCloud* pc, const draco::PointAttribute* pa, DracoInt16Array* out_values) {
  return self->GetAttributeInt16ForAllPoints(*pc, *pa, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeUInt16ForAllPoints_3(Decoder* self, const draco::PointCloud* pc, const draco::PointAttribute* pa, DracoUInt16Array* out_values) {
  return self->GetAttributeUInt16ForAllPoints(*pc, *pa, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeInt32ForAllPoints_3(Decoder* self, const draco::PointCloud* pc, const draco::PointAttribute* pa, DracoInt32Array* out_values) {
  return self->GetAttributeInt32ForAllPoints(*pc, *pa, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeUInt32ForAllPoints_3(Decoder* self, const draco::PointCloud* pc, const draco::PointAttribute* pa, DracoUInt32Array* out_values) {
  return self->GetAttributeUInt32ForAllPoints(*pc, *pa, out_values);
}

bool EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetAttributeDataArrayForAllPoints_5(Decoder* self, const draco::PointCloud* pc, const draco::PointAttribute* pa, draco_DataType data_type, int out_size, void* out_values) {
  return self->GetAttributeDataArrayForAllPoints(*pc, *pa, data_type, out_size, out_values);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_SkipAttributeTransform_1(Decoder* self, draco_GeometryAttribute_Type att_type) {
  self->SkipAttributeTransform(att_type);
}

draco_EncodedGeometryType EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_GetEncodedGeometryType_Deprecated_1(Decoder* self, draco::DecoderBuffer* in_buffer) {
  return self->GetEncodedGeometryType_Deprecated(in_buffer);
}

const draco::Status* EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_DecodeBufferToPointCloud_2(Decoder* self, draco::DecoderBuffer* in_buffer, draco::PointCloud* out_point_cloud) {
  return self->DecodeBufferToPointCloud(in_buffer, out_point_cloud);
}

const draco::Status* EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder_DecodeBufferToMesh_2(Decoder* self, draco::DecoderBuffer* in_buffer, draco::Mesh* out_mesh) {
  return self->DecodeBufferToMesh(in_buffer, out_mesh);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Decoder___destroy___0(Decoder* self) {
  delete self;
}

// $draco_AttributeTransformType
draco_AttributeTransformType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_AttributeTransformType_ATTRIBUTE_INVALID_TRANSFORM() {
  return draco::ATTRIBUTE_INVALID_TRANSFORM;
}
draco_AttributeTransformType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_AttributeTransformType_ATTRIBUTE_NO_TRANSFORM() {
  return draco::ATTRIBUTE_NO_TRANSFORM;
}
draco_AttributeTransformType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_AttributeTransformType_ATTRIBUTE_QUANTIZATION_TRANSFORM() {
  return draco::ATTRIBUTE_QUANTIZATION_TRANSFORM;
}
draco_AttributeTransformType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_AttributeTransformType_ATTRIBUTE_OCTAHEDRON_TRANSFORM() {
  return draco::ATTRIBUTE_OCTAHEDRON_TRANSFORM;
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

// $draco_DataType
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_INVALID() {
  return draco::DT_INVALID;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_INT8() {
  return draco::DT_INT8;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_UINT8() {
  return draco::DT_UINT8;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_INT16() {
  return draco::DT_INT16;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_UINT16() {
  return draco::DT_UINT16;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_INT32() {
  return draco::DT_INT32;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_UINT32() {
  return draco::DT_UINT32;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_INT64() {
  return draco::DT_INT64;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_UINT64() {
  return draco::DT_UINT64;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_FLOAT32() {
  return draco::DT_FLOAT32;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_FLOAT64() {
  return draco::DT_FLOAT64;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_BOOL() {
  return draco::DT_BOOL;
}
draco_DataType EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_DataType_DT_TYPES_COUNT() {
  return draco::DT_TYPES_COUNT;
}

// $draco_StatusCode
draco_StatusCode EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_StatusCode_OK() {
  return draco_Status::OK;
}
draco_StatusCode EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_StatusCode_DRACO_ERROR() {
  return draco_Status::DRACO_ERROR;
}
draco_StatusCode EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_StatusCode_IO_ERROR() {
  return draco_Status::IO_ERROR;
}
draco_StatusCode EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_StatusCode_INVALID_PARAMETER() {
  return draco_Status::INVALID_PARAMETER;
}
draco_StatusCode EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_StatusCode_UNSUPPORTED_VERSION() {
  return draco_Status::UNSUPPORTED_VERSION;
}
draco_StatusCode EMSCRIPTEN_KEEPALIVE emscripten_enum_draco_StatusCode_UNKNOWN_VERSION() {
  return draco_Status::UNKNOWN_VERSION;
}

}

