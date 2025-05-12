
// Bindings utilities

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function WrapperObject() {
}
WrapperObject.prototype = Object.create(WrapperObject.prototype);
WrapperObject.prototype.constructor = WrapperObject;
WrapperObject.prototype.__class__ = WrapperObject;
WrapperObject.__cache__ = {};
Module['WrapperObject'] = WrapperObject;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant)
    @param {*=} __class__ */
function getCache(__class__) {
  return (__class__ || WrapperObject).__cache__;
}
Module['getCache'] = getCache;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant)
    @param {*=} __class__ */
function wrapPointer(ptr, __class__) {
  var cache = getCache(__class__);
  var ret = cache[ptr];
  if (ret) return ret;
  ret = Object.create((__class__ || WrapperObject).prototype);
  ret.ptr = ptr;
  return cache[ptr] = ret;
}
Module['wrapPointer'] = wrapPointer;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function castObject(obj, __class__) {
  return wrapPointer(obj.ptr, __class__);
}
Module['castObject'] = castObject;

Module['NULL'] = wrapPointer(0);

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function destroy(obj) {
  if (!obj['__destroy__']) throw 'Error: Cannot destroy object. (Did you create it yourself?)';
  obj['__destroy__']();
  // Remove from cache, so the object can be GC'd and refs added onto it released
  delete getCache(obj.__class__)[obj.ptr];
}
Module['destroy'] = destroy;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function compare(obj1, obj2) {
  return obj1.ptr === obj2.ptr;
}
Module['compare'] = compare;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function getPointer(obj) {
  return obj.ptr;
}
Module['getPointer'] = getPointer;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function getClass(obj) {
  return obj.__class__;
}
Module['getClass'] = getClass;

// Converts big (string or array) values into a C-style storage, in temporary space

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
var ensureCache = {
  buffer: 0,  // the main buffer of temporary storage
  size: 0,   // the size of buffer
  pos: 0,    // the next free offset in buffer
  temps: [], // extra allocations
  needed: 0, // the total size we need next time

  prepare() {
    if (ensureCache.needed) {
      // clear the temps
      for (var i = 0; i < ensureCache.temps.length; i++) {
        Module['_webidl_free'](ensureCache.temps[i]);
      }
      ensureCache.temps.length = 0;
      // prepare to allocate a bigger buffer
      Module['_webidl_free'](ensureCache.buffer);
      ensureCache.buffer = 0;
      ensureCache.size += ensureCache.needed;
      // clean up
      ensureCache.needed = 0;
    }
    if (!ensureCache.buffer) { // happens first time, or when we need to grow
      ensureCache.size += 128; // heuristic, avoid many small grow events
      ensureCache.buffer = Module['_webidl_malloc'](ensureCache.size);
      assert(ensureCache.buffer);
    }
    ensureCache.pos = 0;
  },
  alloc(array, view) {
    assert(ensureCache.buffer);
    var bytes = view.BYTES_PER_ELEMENT;
    var len = array.length * bytes;
    len = alignMemory(len, 8); // keep things aligned to 8 byte boundaries
    var ret;
    if (ensureCache.pos + len >= ensureCache.size) {
      // we failed to allocate in the buffer, ensureCache time around :(
      assert(len > 0); // null terminator, at least
      ensureCache.needed += len;
      ret = Module['_webidl_malloc'](len);
      ensureCache.temps.push(ret);
    } else {
      // we can allocate in the buffer
      ret = ensureCache.buffer + ensureCache.pos;
      ensureCache.pos += len;
    }
    return ret;
  },
  copy(array, view, offset) {
    offset /= view.BYTES_PER_ELEMENT;
    for (var i = 0; i < array.length; i++) {
      view[offset + i] = array[i];
    }
  },
};

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureString(value) {
  if (typeof value === 'string') {
    var intArray = intArrayFromString(value);
    var offset = ensureCache.alloc(intArray, HEAP8);
    ensureCache.copy(intArray, HEAP8, offset);
    return offset;
  }
  return value;
}

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureInt8(value) {
  if (typeof value === 'object') {
    var offset = ensureCache.alloc(value, HEAP8);
    ensureCache.copy(value, HEAP8, offset);
    return offset;
  }
  return value;
}

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureInt16(value) {
  if (typeof value === 'object') {
    var offset = ensureCache.alloc(value, HEAP16);
    ensureCache.copy(value, HEAP16, offset);
    return offset;
  }
  return value;
}

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureInt32(value) {
  if (typeof value === 'object') {
    var offset = ensureCache.alloc(value, HEAP32);
    ensureCache.copy(value, HEAP32, offset);
    return offset;
  }
  return value;
}

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureFloat32(value) {
  if (typeof value === 'object') {
    var offset = ensureCache.alloc(value, HEAPF32);
    ensureCache.copy(value, HEAPF32, offset);
    return offset;
  }
  return value;
}

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureFloat64(value) {
  if (typeof value === 'object') {
    var offset = ensureCache.alloc(value, HEAPF64);
    ensureCache.copy(value, HEAPF64, offset);
    return offset;
  }
  return value;
}

// Interface: VoidPtr

/** @suppress {undefinedVars, duplicate} @this{Object} */
function VoidPtr() { throw "cannot construct a VoidPtr, no constructor in IDL" }
VoidPtr.prototype = Object.create(WrapperObject.prototype);
VoidPtr.prototype.constructor = VoidPtr;
VoidPtr.prototype.__class__ = VoidPtr;
VoidPtr.__cache__ = {};
Module['VoidPtr'] = VoidPtr;

/** @suppress {undefinedVars, duplicate} @this{Object} */
VoidPtr.prototype['__destroy__'] = VoidPtr.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_VoidPtr___destroy___0(self);
};

// Interface: GeometryAttribute

/** @suppress {undefinedVars, duplicate} @this{Object} */
function GeometryAttribute() {
  this.ptr = _emscripten_bind_GeometryAttribute_GeometryAttribute_0();
  getCache(GeometryAttribute)[this.ptr] = this;
};

GeometryAttribute.prototype = Object.create(WrapperObject.prototype);
GeometryAttribute.prototype.constructor = GeometryAttribute;
GeometryAttribute.prototype.__class__ = GeometryAttribute;
GeometryAttribute.__cache__ = {};
Module['GeometryAttribute'] = GeometryAttribute;

/** @suppress {undefinedVars, duplicate} @this{Object} */
GeometryAttribute.prototype['__destroy__'] = GeometryAttribute.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_GeometryAttribute___destroy___0(self);
};

// Interface: PointAttribute

/** @suppress {undefinedVars, duplicate} @this{Object} */
function PointAttribute() {
  this.ptr = _emscripten_bind_PointAttribute_PointAttribute_0();
  getCache(PointAttribute)[this.ptr] = this;
};

PointAttribute.prototype = Object.create(WrapperObject.prototype);
PointAttribute.prototype.constructor = PointAttribute;
PointAttribute.prototype.__class__ = PointAttribute;
PointAttribute.__cache__ = {};
Module['PointAttribute'] = PointAttribute;
/** @suppress {undefinedVars, duplicate} @this{Object} */
PointAttribute.prototype['size'] = PointAttribute.prototype.size = function() {
  var self = this.ptr;
  return _emscripten_bind_PointAttribute_size_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointAttribute.prototype['attribute_type'] = PointAttribute.prototype.attribute_type = function() {
  var self = this.ptr;
  return _emscripten_bind_PointAttribute_attribute_type_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointAttribute.prototype['data_type'] = PointAttribute.prototype.data_type = function() {
  var self = this.ptr;
  return _emscripten_bind_PointAttribute_data_type_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointAttribute.prototype['num_components'] = PointAttribute.prototype.num_components = function() {
  var self = this.ptr;
  return _emscripten_bind_PointAttribute_num_components_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointAttribute.prototype['normalized'] = PointAttribute.prototype.normalized = function() {
  var self = this.ptr;
  return !!(_emscripten_bind_PointAttribute_normalized_0(self));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointAttribute.prototype['byte_stride'] = PointAttribute.prototype.byte_stride = function() {
  var self = this.ptr;
  return _emscripten_bind_PointAttribute_byte_stride_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointAttribute.prototype['byte_offset'] = PointAttribute.prototype.byte_offset = function() {
  var self = this.ptr;
  return _emscripten_bind_PointAttribute_byte_offset_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointAttribute.prototype['unique_id'] = PointAttribute.prototype.unique_id = function() {
  var self = this.ptr;
  return _emscripten_bind_PointAttribute_unique_id_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
PointAttribute.prototype['__destroy__'] = PointAttribute.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_PointAttribute___destroy___0(self);
};

// Interface: PointCloud

/** @suppress {undefinedVars, duplicate} @this{Object} */
function PointCloud() {
  this.ptr = _emscripten_bind_PointCloud_PointCloud_0();
  getCache(PointCloud)[this.ptr] = this;
};

PointCloud.prototype = Object.create(WrapperObject.prototype);
PointCloud.prototype.constructor = PointCloud;
PointCloud.prototype.__class__ = PointCloud;
PointCloud.__cache__ = {};
Module['PointCloud'] = PointCloud;
/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloud.prototype['num_attributes'] = PointCloud.prototype.num_attributes = function() {
  var self = this.ptr;
  return _emscripten_bind_PointCloud_num_attributes_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloud.prototype['num_points'] = PointCloud.prototype.num_points = function() {
  var self = this.ptr;
  return _emscripten_bind_PointCloud_num_points_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloud.prototype['__destroy__'] = PointCloud.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_PointCloud___destroy___0(self);
};

// Interface: Mesh

/** @suppress {undefinedVars, duplicate} @this{Object} */
function Mesh() {
  this.ptr = _emscripten_bind_Mesh_Mesh_0();
  getCache(Mesh)[this.ptr] = this;
};

Mesh.prototype = Object.create(WrapperObject.prototype);
Mesh.prototype.constructor = Mesh;
Mesh.prototype.__class__ = Mesh;
Mesh.__cache__ = {};
Module['Mesh'] = Mesh;
/** @suppress {undefinedVars, duplicate} @this{Object} */
Mesh.prototype['num_faces'] = Mesh.prototype.num_faces = function() {
  var self = this.ptr;
  return _emscripten_bind_Mesh_num_faces_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Mesh.prototype['num_attributes'] = Mesh.prototype.num_attributes = function() {
  var self = this.ptr;
  return _emscripten_bind_Mesh_num_attributes_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Mesh.prototype['num_points'] = Mesh.prototype.num_points = function() {
  var self = this.ptr;
  return _emscripten_bind_Mesh_num_points_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Mesh.prototype['set_num_points'] = Mesh.prototype.set_num_points = function(num_points) {
  var self = this.ptr;
  if (num_points && typeof num_points === 'object') num_points = num_points.ptr;
  _emscripten_bind_Mesh_set_num_points_1(self, num_points);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
Mesh.prototype['__destroy__'] = Mesh.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_Mesh___destroy___0(self);
};

// Interface: Metadata

/** @suppress {undefinedVars, duplicate} @this{Object} */
function Metadata() {
  this.ptr = _emscripten_bind_Metadata_Metadata_0();
  getCache(Metadata)[this.ptr] = this;
};

Metadata.prototype = Object.create(WrapperObject.prototype);
Metadata.prototype.constructor = Metadata;
Metadata.prototype.__class__ = Metadata;
Metadata.__cache__ = {};
Module['Metadata'] = Metadata;

/** @suppress {undefinedVars, duplicate} @this{Object} */
Metadata.prototype['__destroy__'] = Metadata.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_Metadata___destroy___0(self);
};

// Interface: DracoInt8Array

/** @suppress {undefinedVars, duplicate} @this{Object} */
function DracoInt8Array() {
  this.ptr = _emscripten_bind_DracoInt8Array_DracoInt8Array_0();
  getCache(DracoInt8Array)[this.ptr] = this;
};

DracoInt8Array.prototype = Object.create(WrapperObject.prototype);
DracoInt8Array.prototype.constructor = DracoInt8Array;
DracoInt8Array.prototype.__class__ = DracoInt8Array;
DracoInt8Array.__cache__ = {};
Module['DracoInt8Array'] = DracoInt8Array;
/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoInt8Array.prototype['GetValue'] = DracoInt8Array.prototype.GetValue = function(index) {
  var self = this.ptr;
  if (index && typeof index === 'object') index = index.ptr;
  return _emscripten_bind_DracoInt8Array_GetValue_1(self, index);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoInt8Array.prototype['size'] = DracoInt8Array.prototype.size = function() {
  var self = this.ptr;
  return _emscripten_bind_DracoInt8Array_size_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoInt8Array.prototype['__destroy__'] = DracoInt8Array.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_DracoInt8Array___destroy___0(self);
};

// Interface: MetadataBuilder

/** @suppress {undefinedVars, duplicate} @this{Object} */
function MetadataBuilder() {
  this.ptr = _emscripten_bind_MetadataBuilder_MetadataBuilder_0();
  getCache(MetadataBuilder)[this.ptr] = this;
};

MetadataBuilder.prototype = Object.create(WrapperObject.prototype);
MetadataBuilder.prototype.constructor = MetadataBuilder;
MetadataBuilder.prototype.__class__ = MetadataBuilder;
MetadataBuilder.__cache__ = {};
Module['MetadataBuilder'] = MetadataBuilder;
/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataBuilder.prototype['AddStringEntry'] = MetadataBuilder.prototype.AddStringEntry = function(metadata, entry_name, entry_value) {
  var self = this.ptr;
  ensureCache.prepare();
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  if (entry_name && typeof entry_name === 'object') entry_name = entry_name.ptr;
  else entry_name = ensureString(entry_name);
  if (entry_value && typeof entry_value === 'object') entry_value = entry_value.ptr;
  else entry_value = ensureString(entry_value);
  return !!(_emscripten_bind_MetadataBuilder_AddStringEntry_3(self, metadata, entry_name, entry_value));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataBuilder.prototype['AddIntEntry'] = MetadataBuilder.prototype.AddIntEntry = function(metadata, entry_name, entry_value) {
  var self = this.ptr;
  ensureCache.prepare();
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  if (entry_name && typeof entry_name === 'object') entry_name = entry_name.ptr;
  else entry_name = ensureString(entry_name);
  if (entry_value && typeof entry_value === 'object') entry_value = entry_value.ptr;
  return !!(_emscripten_bind_MetadataBuilder_AddIntEntry_3(self, metadata, entry_name, entry_value));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataBuilder.prototype['AddIntEntryArray'] = MetadataBuilder.prototype.AddIntEntryArray = function(metadata, entry_name, att_values, num_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  if (entry_name && typeof entry_name === 'object') entry_name = entry_name.ptr;
  else entry_name = ensureString(entry_name);
  if (typeof att_values == 'object') { att_values = ensureInt32(att_values); }
  if (num_values && typeof num_values === 'object') num_values = num_values.ptr;
  return !!(_emscripten_bind_MetadataBuilder_AddIntEntryArray_4(self, metadata, entry_name, att_values, num_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataBuilder.prototype['AddDoubleEntry'] = MetadataBuilder.prototype.AddDoubleEntry = function(metadata, entry_name, entry_value) {
  var self = this.ptr;
  ensureCache.prepare();
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  if (entry_name && typeof entry_name === 'object') entry_name = entry_name.ptr;
  else entry_name = ensureString(entry_name);
  if (entry_value && typeof entry_value === 'object') entry_value = entry_value.ptr;
  return !!(_emscripten_bind_MetadataBuilder_AddDoubleEntry_3(self, metadata, entry_name, entry_value));
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataBuilder.prototype['__destroy__'] = MetadataBuilder.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_MetadataBuilder___destroy___0(self);
};

// Interface: PointCloudBuilder

/** @suppress {undefinedVars, duplicate} @this{Object} */
function PointCloudBuilder() {
  this.ptr = _emscripten_bind_PointCloudBuilder_PointCloudBuilder_0();
  getCache(PointCloudBuilder)[this.ptr] = this;
};

PointCloudBuilder.prototype = Object.create(WrapperObject.prototype);
PointCloudBuilder.prototype.constructor = PointCloudBuilder;
PointCloudBuilder.prototype.__class__ = PointCloudBuilder;
PointCloudBuilder.__cache__ = {};
Module['PointCloudBuilder'] = PointCloudBuilder;
/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['AddFloatAttribute'] = PointCloudBuilder.prototype.AddFloatAttribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureFloat32(att_values); }
  return _emscripten_bind_PointCloudBuilder_AddFloatAttribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['AddInt8Attribute'] = PointCloudBuilder.prototype.AddInt8Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt8(att_values); }
  return _emscripten_bind_PointCloudBuilder_AddInt8Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['AddUInt8Attribute'] = PointCloudBuilder.prototype.AddUInt8Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt8(att_values); }
  return _emscripten_bind_PointCloudBuilder_AddUInt8Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['AddInt16Attribute'] = PointCloudBuilder.prototype.AddInt16Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt16(att_values); }
  return _emscripten_bind_PointCloudBuilder_AddInt16Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['AddUInt16Attribute'] = PointCloudBuilder.prototype.AddUInt16Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt16(att_values); }
  return _emscripten_bind_PointCloudBuilder_AddUInt16Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['AddInt32Attribute'] = PointCloudBuilder.prototype.AddInt32Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt32(att_values); }
  return _emscripten_bind_PointCloudBuilder_AddInt32Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['AddUInt32Attribute'] = PointCloudBuilder.prototype.AddUInt32Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt32(att_values); }
  return _emscripten_bind_PointCloudBuilder_AddUInt32Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['AddMetadata'] = PointCloudBuilder.prototype.AddMetadata = function(pc, metadata) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  return !!(_emscripten_bind_PointCloudBuilder_AddMetadata_2(self, pc, metadata));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['SetMetadataForAttribute'] = PointCloudBuilder.prototype.SetMetadataForAttribute = function(pc, attribute_id, metadata) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (attribute_id && typeof attribute_id === 'object') attribute_id = attribute_id.ptr;
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  return !!(_emscripten_bind_PointCloudBuilder_SetMetadataForAttribute_3(self, pc, attribute_id, metadata));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['SetNormalizedFlagForAttribute'] = PointCloudBuilder.prototype.SetNormalizedFlagForAttribute = function(pc, attribute_id, normalized) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (attribute_id && typeof attribute_id === 'object') attribute_id = attribute_id.ptr;
  if (normalized && typeof normalized === 'object') normalized = normalized.ptr;
  return !!(_emscripten_bind_PointCloudBuilder_SetNormalizedFlagForAttribute_3(self, pc, attribute_id, normalized));
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
PointCloudBuilder.prototype['__destroy__'] = PointCloudBuilder.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_PointCloudBuilder___destroy___0(self);
};

// Interface: MeshBuilder

/** @suppress {undefinedVars, duplicate} @this{Object} */
function MeshBuilder() {
  this.ptr = _emscripten_bind_MeshBuilder_MeshBuilder_0();
  getCache(MeshBuilder)[this.ptr] = this;
};

MeshBuilder.prototype = Object.create(WrapperObject.prototype);
MeshBuilder.prototype.constructor = MeshBuilder;
MeshBuilder.prototype.__class__ = MeshBuilder;
MeshBuilder.__cache__ = {};
Module['MeshBuilder'] = MeshBuilder;
/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddFacesToMesh'] = MeshBuilder.prototype.AddFacesToMesh = function(mesh, num_faces, faces) {
  var self = this.ptr;
  ensureCache.prepare();
  if (mesh && typeof mesh === 'object') mesh = mesh.ptr;
  if (num_faces && typeof num_faces === 'object') num_faces = num_faces.ptr;
  if (typeof faces == 'object') { faces = ensureInt32(faces); }
  return !!(_emscripten_bind_MeshBuilder_AddFacesToMesh_3(self, mesh, num_faces, faces));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddFloatAttributeToMesh'] = MeshBuilder.prototype.AddFloatAttributeToMesh = function(mesh, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (mesh && typeof mesh === 'object') mesh = mesh.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureFloat32(att_values); }
  return _emscripten_bind_MeshBuilder_AddFloatAttributeToMesh_5(self, mesh, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddInt32AttributeToMesh'] = MeshBuilder.prototype.AddInt32AttributeToMesh = function(mesh, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (mesh && typeof mesh === 'object') mesh = mesh.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt32(att_values); }
  return _emscripten_bind_MeshBuilder_AddInt32AttributeToMesh_5(self, mesh, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddMetadataToMesh'] = MeshBuilder.prototype.AddMetadataToMesh = function(mesh, metadata) {
  var self = this.ptr;
  if (mesh && typeof mesh === 'object') mesh = mesh.ptr;
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  return !!(_emscripten_bind_MeshBuilder_AddMetadataToMesh_2(self, mesh, metadata));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddFloatAttribute'] = MeshBuilder.prototype.AddFloatAttribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureFloat32(att_values); }
  return _emscripten_bind_MeshBuilder_AddFloatAttribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddInt8Attribute'] = MeshBuilder.prototype.AddInt8Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt8(att_values); }
  return _emscripten_bind_MeshBuilder_AddInt8Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddUInt8Attribute'] = MeshBuilder.prototype.AddUInt8Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt8(att_values); }
  return _emscripten_bind_MeshBuilder_AddUInt8Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddInt16Attribute'] = MeshBuilder.prototype.AddInt16Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt16(att_values); }
  return _emscripten_bind_MeshBuilder_AddInt16Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddUInt16Attribute'] = MeshBuilder.prototype.AddUInt16Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt16(att_values); }
  return _emscripten_bind_MeshBuilder_AddUInt16Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddInt32Attribute'] = MeshBuilder.prototype.AddInt32Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt32(att_values); }
  return _emscripten_bind_MeshBuilder_AddInt32Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddUInt32Attribute'] = MeshBuilder.prototype.AddUInt32Attribute = function(pc, type, num_vertices, num_components, att_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (num_vertices && typeof num_vertices === 'object') num_vertices = num_vertices.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof att_values == 'object') { att_values = ensureInt32(att_values); }
  return _emscripten_bind_MeshBuilder_AddUInt32Attribute_5(self, pc, type, num_vertices, num_components, att_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['AddMetadata'] = MeshBuilder.prototype.AddMetadata = function(pc, metadata) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  return !!(_emscripten_bind_MeshBuilder_AddMetadata_2(self, pc, metadata));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['SetMetadataForAttribute'] = MeshBuilder.prototype.SetMetadataForAttribute = function(pc, attribute_id, metadata) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (attribute_id && typeof attribute_id === 'object') attribute_id = attribute_id.ptr;
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  return !!(_emscripten_bind_MeshBuilder_SetMetadataForAttribute_3(self, pc, attribute_id, metadata));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['SetNormalizedFlagForAttribute'] = MeshBuilder.prototype.SetNormalizedFlagForAttribute = function(pc, attribute_id, normalized) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (attribute_id && typeof attribute_id === 'object') attribute_id = attribute_id.ptr;
  if (normalized && typeof normalized === 'object') normalized = normalized.ptr;
  return !!(_emscripten_bind_MeshBuilder_SetNormalizedFlagForAttribute_3(self, pc, attribute_id, normalized));
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
MeshBuilder.prototype['__destroy__'] = MeshBuilder.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_MeshBuilder___destroy___0(self);
};

// Interface: Encoder

/** @suppress {undefinedVars, duplicate} @this{Object} */
function Encoder() {
  this.ptr = _emscripten_bind_Encoder_Encoder_0();
  getCache(Encoder)[this.ptr] = this;
};

Encoder.prototype = Object.create(WrapperObject.prototype);
Encoder.prototype.constructor = Encoder;
Encoder.prototype.__class__ = Encoder;
Encoder.__cache__ = {};
Module['Encoder'] = Encoder;
/** @suppress {undefinedVars, duplicate} @this{Object} */
Encoder.prototype['SetEncodingMethod'] = Encoder.prototype.SetEncodingMethod = function(method) {
  var self = this.ptr;
  if (method && typeof method === 'object') method = method.ptr;
  _emscripten_bind_Encoder_SetEncodingMethod_1(self, method);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Encoder.prototype['SetAttributeQuantization'] = Encoder.prototype.SetAttributeQuantization = function(type, quantization_bits) {
  var self = this.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  if (quantization_bits && typeof quantization_bits === 'object') quantization_bits = quantization_bits.ptr;
  _emscripten_bind_Encoder_SetAttributeQuantization_2(self, type, quantization_bits);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Encoder.prototype['SetAttributeExplicitQuantization'] = Encoder.prototype.SetAttributeExplicitQuantization = function(type, quantization_bits, num_components, origin, range) {
  var self = this.ptr;
  ensureCache.prepare();
  if (type && typeof type === 'object') type = type.ptr;
  if (quantization_bits && typeof quantization_bits === 'object') quantization_bits = quantization_bits.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof origin == 'object') { origin = ensureFloat32(origin); }
  if (range && typeof range === 'object') range = range.ptr;
  _emscripten_bind_Encoder_SetAttributeExplicitQuantization_5(self, type, quantization_bits, num_components, origin, range);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Encoder.prototype['SetSpeedOptions'] = Encoder.prototype.SetSpeedOptions = function(encoding_speed, decoding_speed) {
  var self = this.ptr;
  if (encoding_speed && typeof encoding_speed === 'object') encoding_speed = encoding_speed.ptr;
  if (decoding_speed && typeof decoding_speed === 'object') decoding_speed = decoding_speed.ptr;
  _emscripten_bind_Encoder_SetSpeedOptions_2(self, encoding_speed, decoding_speed);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Encoder.prototype['SetTrackEncodedProperties'] = Encoder.prototype.SetTrackEncodedProperties = function(flag) {
  var self = this.ptr;
  if (flag && typeof flag === 'object') flag = flag.ptr;
  _emscripten_bind_Encoder_SetTrackEncodedProperties_1(self, flag);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Encoder.prototype['EncodeMeshToDracoBuffer'] = Encoder.prototype.EncodeMeshToDracoBuffer = function(mesh, encoded_data) {
  var self = this.ptr;
  if (mesh && typeof mesh === 'object') mesh = mesh.ptr;
  if (encoded_data && typeof encoded_data === 'object') encoded_data = encoded_data.ptr;
  return _emscripten_bind_Encoder_EncodeMeshToDracoBuffer_2(self, mesh, encoded_data);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Encoder.prototype['EncodePointCloudToDracoBuffer'] = Encoder.prototype.EncodePointCloudToDracoBuffer = function(pc, deduplicate_values, encoded_data) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (deduplicate_values && typeof deduplicate_values === 'object') deduplicate_values = deduplicate_values.ptr;
  if (encoded_data && typeof encoded_data === 'object') encoded_data = encoded_data.ptr;
  return _emscripten_bind_Encoder_EncodePointCloudToDracoBuffer_3(self, pc, deduplicate_values, encoded_data);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Encoder.prototype['GetNumberOfEncodedPoints'] = Encoder.prototype.GetNumberOfEncodedPoints = function() {
  var self = this.ptr;
  return _emscripten_bind_Encoder_GetNumberOfEncodedPoints_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Encoder.prototype['GetNumberOfEncodedFaces'] = Encoder.prototype.GetNumberOfEncodedFaces = function() {
  var self = this.ptr;
  return _emscripten_bind_Encoder_GetNumberOfEncodedFaces_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
Encoder.prototype['__destroy__'] = Encoder.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_Encoder___destroy___0(self);
};

// Interface: ExpertEncoder

/** @suppress {undefinedVars, duplicate} @this{Object} */
function ExpertEncoder(pc) {
  if (pc && typeof pc === 'object') pc = pc.ptr;
  this.ptr = _emscripten_bind_ExpertEncoder_ExpertEncoder_1(pc);
  getCache(ExpertEncoder)[this.ptr] = this;
};

ExpertEncoder.prototype = Object.create(WrapperObject.prototype);
ExpertEncoder.prototype.constructor = ExpertEncoder;
ExpertEncoder.prototype.__class__ = ExpertEncoder;
ExpertEncoder.__cache__ = {};
Module['ExpertEncoder'] = ExpertEncoder;
/** @suppress {undefinedVars, duplicate} @this{Object} */
ExpertEncoder.prototype['SetEncodingMethod'] = ExpertEncoder.prototype.SetEncodingMethod = function(method) {
  var self = this.ptr;
  if (method && typeof method === 'object') method = method.ptr;
  _emscripten_bind_ExpertEncoder_SetEncodingMethod_1(self, method);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
ExpertEncoder.prototype['SetAttributeQuantization'] = ExpertEncoder.prototype.SetAttributeQuantization = function(att_id, quantization_bits) {
  var self = this.ptr;
  if (att_id && typeof att_id === 'object') att_id = att_id.ptr;
  if (quantization_bits && typeof quantization_bits === 'object') quantization_bits = quantization_bits.ptr;
  _emscripten_bind_ExpertEncoder_SetAttributeQuantization_2(self, att_id, quantization_bits);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
ExpertEncoder.prototype['SetAttributeExplicitQuantization'] = ExpertEncoder.prototype.SetAttributeExplicitQuantization = function(att_id, quantization_bits, num_components, origin, range) {
  var self = this.ptr;
  ensureCache.prepare();
  if (att_id && typeof att_id === 'object') att_id = att_id.ptr;
  if (quantization_bits && typeof quantization_bits === 'object') quantization_bits = quantization_bits.ptr;
  if (num_components && typeof num_components === 'object') num_components = num_components.ptr;
  if (typeof origin == 'object') { origin = ensureFloat32(origin); }
  if (range && typeof range === 'object') range = range.ptr;
  _emscripten_bind_ExpertEncoder_SetAttributeExplicitQuantization_5(self, att_id, quantization_bits, num_components, origin, range);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
ExpertEncoder.prototype['SetSpeedOptions'] = ExpertEncoder.prototype.SetSpeedOptions = function(encoding_speed, decoding_speed) {
  var self = this.ptr;
  if (encoding_speed && typeof encoding_speed === 'object') encoding_speed = encoding_speed.ptr;
  if (decoding_speed && typeof decoding_speed === 'object') decoding_speed = decoding_speed.ptr;
  _emscripten_bind_ExpertEncoder_SetSpeedOptions_2(self, encoding_speed, decoding_speed);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
ExpertEncoder.prototype['SetTrackEncodedProperties'] = ExpertEncoder.prototype.SetTrackEncodedProperties = function(flag) {
  var self = this.ptr;
  if (flag && typeof flag === 'object') flag = flag.ptr;
  _emscripten_bind_ExpertEncoder_SetTrackEncodedProperties_1(self, flag);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
ExpertEncoder.prototype['EncodeToDracoBuffer'] = ExpertEncoder.prototype.EncodeToDracoBuffer = function(deduplicate_values, encoded_data) {
  var self = this.ptr;
  if (deduplicate_values && typeof deduplicate_values === 'object') deduplicate_values = deduplicate_values.ptr;
  if (encoded_data && typeof encoded_data === 'object') encoded_data = encoded_data.ptr;
  return _emscripten_bind_ExpertEncoder_EncodeToDracoBuffer_2(self, deduplicate_values, encoded_data);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
ExpertEncoder.prototype['GetNumberOfEncodedPoints'] = ExpertEncoder.prototype.GetNumberOfEncodedPoints = function() {
  var self = this.ptr;
  return _emscripten_bind_ExpertEncoder_GetNumberOfEncodedPoints_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
ExpertEncoder.prototype['GetNumberOfEncodedFaces'] = ExpertEncoder.prototype.GetNumberOfEncodedFaces = function() {
  var self = this.ptr;
  return _emscripten_bind_ExpertEncoder_GetNumberOfEncodedFaces_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
ExpertEncoder.prototype['__destroy__'] = ExpertEncoder.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_ExpertEncoder___destroy___0(self);
};

(function() {
  function setupEnums() {
    
// $draco_GeometryAttribute_Type

    Module['INVALID'] = _emscripten_enum_draco_GeometryAttribute_Type_INVALID();

    Module['POSITION'] = _emscripten_enum_draco_GeometryAttribute_Type_POSITION();

    Module['NORMAL'] = _emscripten_enum_draco_GeometryAttribute_Type_NORMAL();

    Module['COLOR'] = _emscripten_enum_draco_GeometryAttribute_Type_COLOR();

    Module['TEX_COORD'] = _emscripten_enum_draco_GeometryAttribute_Type_TEX_COORD();

    Module['GENERIC'] = _emscripten_enum_draco_GeometryAttribute_Type_GENERIC();

    
// $draco_EncodedGeometryType

    Module['INVALID_GEOMETRY_TYPE'] = _emscripten_enum_draco_EncodedGeometryType_INVALID_GEOMETRY_TYPE();

    Module['POINT_CLOUD'] = _emscripten_enum_draco_EncodedGeometryType_POINT_CLOUD();

    Module['TRIANGULAR_MESH'] = _emscripten_enum_draco_EncodedGeometryType_TRIANGULAR_MESH();

    
// $draco_MeshEncoderMethod

    Module['MESH_SEQUENTIAL_ENCODING'] = _emscripten_enum_draco_MeshEncoderMethod_MESH_SEQUENTIAL_ENCODING();

    Module['MESH_EDGEBREAKER_ENCODING'] = _emscripten_enum_draco_MeshEncoderMethod_MESH_EDGEBREAKER_ENCODING();

  }
  if (runtimeInitialized) setupEnums();
  else addOnInit(setupEnums);
})();
