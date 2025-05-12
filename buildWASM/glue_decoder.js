
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

// Interface: DecoderBuffer

/** @suppress {undefinedVars, duplicate} @this{Object} */
function DecoderBuffer() {
  this.ptr = _emscripten_bind_DecoderBuffer_DecoderBuffer_0();
  getCache(DecoderBuffer)[this.ptr] = this;
};

DecoderBuffer.prototype = Object.create(WrapperObject.prototype);
DecoderBuffer.prototype.constructor = DecoderBuffer;
DecoderBuffer.prototype.__class__ = DecoderBuffer;
DecoderBuffer.__cache__ = {};
Module['DecoderBuffer'] = DecoderBuffer;
/** @suppress {undefinedVars, duplicate} @this{Object} */
DecoderBuffer.prototype['Init'] = DecoderBuffer.prototype.Init = function(data, data_size) {
  var self = this.ptr;
  ensureCache.prepare();
  if (typeof data == 'object') { data = ensureInt8(data); }
  if (data_size && typeof data_size === 'object') data_size = data_size.ptr;
  _emscripten_bind_DecoderBuffer_Init_2(self, data, data_size);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
DecoderBuffer.prototype['__destroy__'] = DecoderBuffer.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_DecoderBuffer___destroy___0(self);
};

// Interface: AttributeTransformData

/** @suppress {undefinedVars, duplicate} @this{Object} */
function AttributeTransformData() {
  this.ptr = _emscripten_bind_AttributeTransformData_AttributeTransformData_0();
  getCache(AttributeTransformData)[this.ptr] = this;
};

AttributeTransformData.prototype = Object.create(WrapperObject.prototype);
AttributeTransformData.prototype.constructor = AttributeTransformData;
AttributeTransformData.prototype.__class__ = AttributeTransformData;
AttributeTransformData.__cache__ = {};
Module['AttributeTransformData'] = AttributeTransformData;
/** @suppress {undefinedVars, duplicate} @this{Object} */
AttributeTransformData.prototype['transform_type'] = AttributeTransformData.prototype.transform_type = function() {
  var self = this.ptr;
  return _emscripten_bind_AttributeTransformData_transform_type_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
AttributeTransformData.prototype['__destroy__'] = AttributeTransformData.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_AttributeTransformData___destroy___0(self);
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
PointAttribute.prototype['GetAttributeTransformData'] = PointAttribute.prototype.GetAttributeTransformData = function() {
  var self = this.ptr;
  return wrapPointer(_emscripten_bind_PointAttribute_GetAttributeTransformData_0(self), AttributeTransformData);
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

// Interface: AttributeQuantizationTransform

/** @suppress {undefinedVars, duplicate} @this{Object} */
function AttributeQuantizationTransform() {
  this.ptr = _emscripten_bind_AttributeQuantizationTransform_AttributeQuantizationTransform_0();
  getCache(AttributeQuantizationTransform)[this.ptr] = this;
};

AttributeQuantizationTransform.prototype = Object.create(WrapperObject.prototype);
AttributeQuantizationTransform.prototype.constructor = AttributeQuantizationTransform;
AttributeQuantizationTransform.prototype.__class__ = AttributeQuantizationTransform;
AttributeQuantizationTransform.__cache__ = {};
Module['AttributeQuantizationTransform'] = AttributeQuantizationTransform;
/** @suppress {undefinedVars, duplicate} @this{Object} */
AttributeQuantizationTransform.prototype['InitFromAttribute'] = AttributeQuantizationTransform.prototype.InitFromAttribute = function(att) {
  var self = this.ptr;
  if (att && typeof att === 'object') att = att.ptr;
  return !!(_emscripten_bind_AttributeQuantizationTransform_InitFromAttribute_1(self, att));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
AttributeQuantizationTransform.prototype['quantization_bits'] = AttributeQuantizationTransform.prototype.quantization_bits = function() {
  var self = this.ptr;
  return _emscripten_bind_AttributeQuantizationTransform_quantization_bits_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
AttributeQuantizationTransform.prototype['min_value'] = AttributeQuantizationTransform.prototype.min_value = function(axis) {
  var self = this.ptr;
  if (axis && typeof axis === 'object') axis = axis.ptr;
  return _emscripten_bind_AttributeQuantizationTransform_min_value_1(self, axis);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
AttributeQuantizationTransform.prototype['range'] = AttributeQuantizationTransform.prototype.range = function() {
  var self = this.ptr;
  return _emscripten_bind_AttributeQuantizationTransform_range_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
AttributeQuantizationTransform.prototype['__destroy__'] = AttributeQuantizationTransform.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_AttributeQuantizationTransform___destroy___0(self);
};

// Interface: AttributeOctahedronTransform

/** @suppress {undefinedVars, duplicate} @this{Object} */
function AttributeOctahedronTransform() {
  this.ptr = _emscripten_bind_AttributeOctahedronTransform_AttributeOctahedronTransform_0();
  getCache(AttributeOctahedronTransform)[this.ptr] = this;
};

AttributeOctahedronTransform.prototype = Object.create(WrapperObject.prototype);
AttributeOctahedronTransform.prototype.constructor = AttributeOctahedronTransform;
AttributeOctahedronTransform.prototype.__class__ = AttributeOctahedronTransform;
AttributeOctahedronTransform.__cache__ = {};
Module['AttributeOctahedronTransform'] = AttributeOctahedronTransform;
/** @suppress {undefinedVars, duplicate} @this{Object} */
AttributeOctahedronTransform.prototype['InitFromAttribute'] = AttributeOctahedronTransform.prototype.InitFromAttribute = function(att) {
  var self = this.ptr;
  if (att && typeof att === 'object') att = att.ptr;
  return !!(_emscripten_bind_AttributeOctahedronTransform_InitFromAttribute_1(self, att));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
AttributeOctahedronTransform.prototype['quantization_bits'] = AttributeOctahedronTransform.prototype.quantization_bits = function() {
  var self = this.ptr;
  return _emscripten_bind_AttributeOctahedronTransform_quantization_bits_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
AttributeOctahedronTransform.prototype['__destroy__'] = AttributeOctahedronTransform.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_AttributeOctahedronTransform___destroy___0(self);
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

// Interface: Status

/** @suppress {undefinedVars, duplicate} @this{Object} */
function Status() { throw "cannot construct a Status, no constructor in IDL" }
Status.prototype = Object.create(WrapperObject.prototype);
Status.prototype.constructor = Status;
Status.prototype.__class__ = Status;
Status.__cache__ = {};
Module['Status'] = Status;
/** @suppress {undefinedVars, duplicate} @this{Object} */
Status.prototype['code'] = Status.prototype.code = function() {
  var self = this.ptr;
  return _emscripten_bind_Status_code_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Status.prototype['ok'] = Status.prototype.ok = function() {
  var self = this.ptr;
  return !!(_emscripten_bind_Status_ok_0(self));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Status.prototype['error_msg'] = Status.prototype.error_msg = function() {
  var self = this.ptr;
  return UTF8ToString(_emscripten_bind_Status_error_msg_0(self));
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
Status.prototype['__destroy__'] = Status.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_Status___destroy___0(self);
};

// Interface: DracoFloat32Array

/** @suppress {undefinedVars, duplicate} @this{Object} */
function DracoFloat32Array() {
  this.ptr = _emscripten_bind_DracoFloat32Array_DracoFloat32Array_0();
  getCache(DracoFloat32Array)[this.ptr] = this;
};

DracoFloat32Array.prototype = Object.create(WrapperObject.prototype);
DracoFloat32Array.prototype.constructor = DracoFloat32Array;
DracoFloat32Array.prototype.__class__ = DracoFloat32Array;
DracoFloat32Array.__cache__ = {};
Module['DracoFloat32Array'] = DracoFloat32Array;
/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoFloat32Array.prototype['GetValue'] = DracoFloat32Array.prototype.GetValue = function(index) {
  var self = this.ptr;
  if (index && typeof index === 'object') index = index.ptr;
  return _emscripten_bind_DracoFloat32Array_GetValue_1(self, index);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoFloat32Array.prototype['size'] = DracoFloat32Array.prototype.size = function() {
  var self = this.ptr;
  return _emscripten_bind_DracoFloat32Array_size_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoFloat32Array.prototype['__destroy__'] = DracoFloat32Array.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_DracoFloat32Array___destroy___0(self);
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

// Interface: DracoUInt8Array

/** @suppress {undefinedVars, duplicate} @this{Object} */
function DracoUInt8Array() {
  this.ptr = _emscripten_bind_DracoUInt8Array_DracoUInt8Array_0();
  getCache(DracoUInt8Array)[this.ptr] = this;
};

DracoUInt8Array.prototype = Object.create(WrapperObject.prototype);
DracoUInt8Array.prototype.constructor = DracoUInt8Array;
DracoUInt8Array.prototype.__class__ = DracoUInt8Array;
DracoUInt8Array.__cache__ = {};
Module['DracoUInt8Array'] = DracoUInt8Array;
/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoUInt8Array.prototype['GetValue'] = DracoUInt8Array.prototype.GetValue = function(index) {
  var self = this.ptr;
  if (index && typeof index === 'object') index = index.ptr;
  return _emscripten_bind_DracoUInt8Array_GetValue_1(self, index);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoUInt8Array.prototype['size'] = DracoUInt8Array.prototype.size = function() {
  var self = this.ptr;
  return _emscripten_bind_DracoUInt8Array_size_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoUInt8Array.prototype['__destroy__'] = DracoUInt8Array.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_DracoUInt8Array___destroy___0(self);
};

// Interface: DracoInt16Array

/** @suppress {undefinedVars, duplicate} @this{Object} */
function DracoInt16Array() {
  this.ptr = _emscripten_bind_DracoInt16Array_DracoInt16Array_0();
  getCache(DracoInt16Array)[this.ptr] = this;
};

DracoInt16Array.prototype = Object.create(WrapperObject.prototype);
DracoInt16Array.prototype.constructor = DracoInt16Array;
DracoInt16Array.prototype.__class__ = DracoInt16Array;
DracoInt16Array.__cache__ = {};
Module['DracoInt16Array'] = DracoInt16Array;
/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoInt16Array.prototype['GetValue'] = DracoInt16Array.prototype.GetValue = function(index) {
  var self = this.ptr;
  if (index && typeof index === 'object') index = index.ptr;
  return _emscripten_bind_DracoInt16Array_GetValue_1(self, index);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoInt16Array.prototype['size'] = DracoInt16Array.prototype.size = function() {
  var self = this.ptr;
  return _emscripten_bind_DracoInt16Array_size_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoInt16Array.prototype['__destroy__'] = DracoInt16Array.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_DracoInt16Array___destroy___0(self);
};

// Interface: DracoUInt16Array

/** @suppress {undefinedVars, duplicate} @this{Object} */
function DracoUInt16Array() {
  this.ptr = _emscripten_bind_DracoUInt16Array_DracoUInt16Array_0();
  getCache(DracoUInt16Array)[this.ptr] = this;
};

DracoUInt16Array.prototype = Object.create(WrapperObject.prototype);
DracoUInt16Array.prototype.constructor = DracoUInt16Array;
DracoUInt16Array.prototype.__class__ = DracoUInt16Array;
DracoUInt16Array.__cache__ = {};
Module['DracoUInt16Array'] = DracoUInt16Array;
/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoUInt16Array.prototype['GetValue'] = DracoUInt16Array.prototype.GetValue = function(index) {
  var self = this.ptr;
  if (index && typeof index === 'object') index = index.ptr;
  return _emscripten_bind_DracoUInt16Array_GetValue_1(self, index);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoUInt16Array.prototype['size'] = DracoUInt16Array.prototype.size = function() {
  var self = this.ptr;
  return _emscripten_bind_DracoUInt16Array_size_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoUInt16Array.prototype['__destroy__'] = DracoUInt16Array.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_DracoUInt16Array___destroy___0(self);
};

// Interface: DracoInt32Array

/** @suppress {undefinedVars, duplicate} @this{Object} */
function DracoInt32Array() {
  this.ptr = _emscripten_bind_DracoInt32Array_DracoInt32Array_0();
  getCache(DracoInt32Array)[this.ptr] = this;
};

DracoInt32Array.prototype = Object.create(WrapperObject.prototype);
DracoInt32Array.prototype.constructor = DracoInt32Array;
DracoInt32Array.prototype.__class__ = DracoInt32Array;
DracoInt32Array.__cache__ = {};
Module['DracoInt32Array'] = DracoInt32Array;
/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoInt32Array.prototype['GetValue'] = DracoInt32Array.prototype.GetValue = function(index) {
  var self = this.ptr;
  if (index && typeof index === 'object') index = index.ptr;
  return _emscripten_bind_DracoInt32Array_GetValue_1(self, index);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoInt32Array.prototype['size'] = DracoInt32Array.prototype.size = function() {
  var self = this.ptr;
  return _emscripten_bind_DracoInt32Array_size_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoInt32Array.prototype['__destroy__'] = DracoInt32Array.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_DracoInt32Array___destroy___0(self);
};

// Interface: DracoUInt32Array

/** @suppress {undefinedVars, duplicate} @this{Object} */
function DracoUInt32Array() {
  this.ptr = _emscripten_bind_DracoUInt32Array_DracoUInt32Array_0();
  getCache(DracoUInt32Array)[this.ptr] = this;
};

DracoUInt32Array.prototype = Object.create(WrapperObject.prototype);
DracoUInt32Array.prototype.constructor = DracoUInt32Array;
DracoUInt32Array.prototype.__class__ = DracoUInt32Array;
DracoUInt32Array.__cache__ = {};
Module['DracoUInt32Array'] = DracoUInt32Array;
/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoUInt32Array.prototype['GetValue'] = DracoUInt32Array.prototype.GetValue = function(index) {
  var self = this.ptr;
  if (index && typeof index === 'object') index = index.ptr;
  return _emscripten_bind_DracoUInt32Array_GetValue_1(self, index);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoUInt32Array.prototype['size'] = DracoUInt32Array.prototype.size = function() {
  var self = this.ptr;
  return _emscripten_bind_DracoUInt32Array_size_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
DracoUInt32Array.prototype['__destroy__'] = DracoUInt32Array.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_DracoUInt32Array___destroy___0(self);
};

// Interface: MetadataQuerier

/** @suppress {undefinedVars, duplicate} @this{Object} */
function MetadataQuerier() {
  this.ptr = _emscripten_bind_MetadataQuerier_MetadataQuerier_0();
  getCache(MetadataQuerier)[this.ptr] = this;
};

MetadataQuerier.prototype = Object.create(WrapperObject.prototype);
MetadataQuerier.prototype.constructor = MetadataQuerier;
MetadataQuerier.prototype.__class__ = MetadataQuerier;
MetadataQuerier.__cache__ = {};
Module['MetadataQuerier'] = MetadataQuerier;
/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataQuerier.prototype['HasEntry'] = MetadataQuerier.prototype.HasEntry = function(metadata, entry_name) {
  var self = this.ptr;
  ensureCache.prepare();
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  if (entry_name && typeof entry_name === 'object') entry_name = entry_name.ptr;
  else entry_name = ensureString(entry_name);
  return !!(_emscripten_bind_MetadataQuerier_HasEntry_2(self, metadata, entry_name));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataQuerier.prototype['GetIntEntry'] = MetadataQuerier.prototype.GetIntEntry = function(metadata, entry_name) {
  var self = this.ptr;
  ensureCache.prepare();
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  if (entry_name && typeof entry_name === 'object') entry_name = entry_name.ptr;
  else entry_name = ensureString(entry_name);
  return _emscripten_bind_MetadataQuerier_GetIntEntry_2(self, metadata, entry_name);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataQuerier.prototype['GetIntEntryArray'] = MetadataQuerier.prototype.GetIntEntryArray = function(metadata, entry_name, out_values) {
  var self = this.ptr;
  ensureCache.prepare();
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  if (entry_name && typeof entry_name === 'object') entry_name = entry_name.ptr;
  else entry_name = ensureString(entry_name);
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  _emscripten_bind_MetadataQuerier_GetIntEntryArray_3(self, metadata, entry_name, out_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataQuerier.prototype['GetDoubleEntry'] = MetadataQuerier.prototype.GetDoubleEntry = function(metadata, entry_name) {
  var self = this.ptr;
  ensureCache.prepare();
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  if (entry_name && typeof entry_name === 'object') entry_name = entry_name.ptr;
  else entry_name = ensureString(entry_name);
  return _emscripten_bind_MetadataQuerier_GetDoubleEntry_2(self, metadata, entry_name);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataQuerier.prototype['GetStringEntry'] = MetadataQuerier.prototype.GetStringEntry = function(metadata, entry_name) {
  var self = this.ptr;
  ensureCache.prepare();
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  if (entry_name && typeof entry_name === 'object') entry_name = entry_name.ptr;
  else entry_name = ensureString(entry_name);
  return UTF8ToString(_emscripten_bind_MetadataQuerier_GetStringEntry_2(self, metadata, entry_name));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataQuerier.prototype['NumEntries'] = MetadataQuerier.prototype.NumEntries = function(metadata) {
  var self = this.ptr;
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  return _emscripten_bind_MetadataQuerier_NumEntries_1(self, metadata);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataQuerier.prototype['GetEntryName'] = MetadataQuerier.prototype.GetEntryName = function(metadata, entry_id) {
  var self = this.ptr;
  if (metadata && typeof metadata === 'object') metadata = metadata.ptr;
  if (entry_id && typeof entry_id === 'object') entry_id = entry_id.ptr;
  return UTF8ToString(_emscripten_bind_MetadataQuerier_GetEntryName_2(self, metadata, entry_id));
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
MetadataQuerier.prototype['__destroy__'] = MetadataQuerier.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_MetadataQuerier___destroy___0(self);
};

// Interface: Decoder

/** @suppress {undefinedVars, duplicate} @this{Object} */
function Decoder() {
  this.ptr = _emscripten_bind_Decoder_Decoder_0();
  getCache(Decoder)[this.ptr] = this;
};

Decoder.prototype = Object.create(WrapperObject.prototype);
Decoder.prototype.constructor = Decoder;
Decoder.prototype.__class__ = Decoder;
Decoder.__cache__ = {};
Module['Decoder'] = Decoder;
/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['DecodeArrayToPointCloud'] = Decoder.prototype.DecodeArrayToPointCloud = function(data, data_size, out_point_cloud) {
  var self = this.ptr;
  ensureCache.prepare();
  if (typeof data == 'object') { data = ensureInt8(data); }
  if (data_size && typeof data_size === 'object') data_size = data_size.ptr;
  if (out_point_cloud && typeof out_point_cloud === 'object') out_point_cloud = out_point_cloud.ptr;
  return wrapPointer(_emscripten_bind_Decoder_DecodeArrayToPointCloud_3(self, data, data_size, out_point_cloud), Status);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['DecodeArrayToMesh'] = Decoder.prototype.DecodeArrayToMesh = function(data, data_size, out_mesh) {
  var self = this.ptr;
  ensureCache.prepare();
  if (typeof data == 'object') { data = ensureInt8(data); }
  if (data_size && typeof data_size === 'object') data_size = data_size.ptr;
  if (out_mesh && typeof out_mesh === 'object') out_mesh = out_mesh.ptr;
  return wrapPointer(_emscripten_bind_Decoder_DecodeArrayToMesh_3(self, data, data_size, out_mesh), Status);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeId'] = Decoder.prototype.GetAttributeId = function(pc, type) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (type && typeof type === 'object') type = type.ptr;
  return _emscripten_bind_Decoder_GetAttributeId_2(self, pc, type);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeIdByName'] = Decoder.prototype.GetAttributeIdByName = function(pc, name) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (name && typeof name === 'object') name = name.ptr;
  else name = ensureString(name);
  return _emscripten_bind_Decoder_GetAttributeIdByName_2(self, pc, name);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeIdByMetadataEntry'] = Decoder.prototype.GetAttributeIdByMetadataEntry = function(pc, name, value) {
  var self = this.ptr;
  ensureCache.prepare();
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (name && typeof name === 'object') name = name.ptr;
  else name = ensureString(name);
  if (value && typeof value === 'object') value = value.ptr;
  else value = ensureString(value);
  return _emscripten_bind_Decoder_GetAttributeIdByMetadataEntry_3(self, pc, name, value);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttribute'] = Decoder.prototype.GetAttribute = function(pc, att_id) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (att_id && typeof att_id === 'object') att_id = att_id.ptr;
  return wrapPointer(_emscripten_bind_Decoder_GetAttribute_2(self, pc, att_id), PointAttribute);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeByUniqueId'] = Decoder.prototype.GetAttributeByUniqueId = function(pc, unique_id) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (unique_id && typeof unique_id === 'object') unique_id = unique_id.ptr;
  return wrapPointer(_emscripten_bind_Decoder_GetAttributeByUniqueId_2(self, pc, unique_id), PointAttribute);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetMetadata'] = Decoder.prototype.GetMetadata = function(pc) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  return wrapPointer(_emscripten_bind_Decoder_GetMetadata_1(self, pc), Metadata);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeMetadata'] = Decoder.prototype.GetAttributeMetadata = function(pc, att_id) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (att_id && typeof att_id === 'object') att_id = att_id.ptr;
  return wrapPointer(_emscripten_bind_Decoder_GetAttributeMetadata_2(self, pc, att_id), Metadata);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetFaceFromMesh'] = Decoder.prototype.GetFaceFromMesh = function(m, face_id, out_values) {
  var self = this.ptr;
  if (m && typeof m === 'object') m = m.ptr;
  if (face_id && typeof face_id === 'object') face_id = face_id.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetFaceFromMesh_3(self, m, face_id, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetTriangleStripsFromMesh'] = Decoder.prototype.GetTriangleStripsFromMesh = function(m, strip_values) {
  var self = this.ptr;
  if (m && typeof m === 'object') m = m.ptr;
  if (strip_values && typeof strip_values === 'object') strip_values = strip_values.ptr;
  return _emscripten_bind_Decoder_GetTriangleStripsFromMesh_2(self, m, strip_values);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetTrianglesUInt16Array'] = Decoder.prototype.GetTrianglesUInt16Array = function(m, out_size, out_values) {
  var self = this.ptr;
  if (m && typeof m === 'object') m = m.ptr;
  if (out_size && typeof out_size === 'object') out_size = out_size.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetTrianglesUInt16Array_3(self, m, out_size, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetTrianglesUInt32Array'] = Decoder.prototype.GetTrianglesUInt32Array = function(m, out_size, out_values) {
  var self = this.ptr;
  if (m && typeof m === 'object') m = m.ptr;
  if (out_size && typeof out_size === 'object') out_size = out_size.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetTrianglesUInt32Array_3(self, m, out_size, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeFloat'] = Decoder.prototype.GetAttributeFloat = function(pa, att_index, out_values) {
  var self = this.ptr;
  if (pa && typeof pa === 'object') pa = pa.ptr;
  if (att_index && typeof att_index === 'object') att_index = att_index.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetAttributeFloat_3(self, pa, att_index, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeFloatForAllPoints'] = Decoder.prototype.GetAttributeFloatForAllPoints = function(pc, pa, out_values) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (pa && typeof pa === 'object') pa = pa.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetAttributeFloatForAllPoints_3(self, pc, pa, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeIntForAllPoints'] = Decoder.prototype.GetAttributeIntForAllPoints = function(pc, pa, out_values) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (pa && typeof pa === 'object') pa = pa.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetAttributeIntForAllPoints_3(self, pc, pa, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeInt8ForAllPoints'] = Decoder.prototype.GetAttributeInt8ForAllPoints = function(pc, pa, out_values) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (pa && typeof pa === 'object') pa = pa.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetAttributeInt8ForAllPoints_3(self, pc, pa, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeUInt8ForAllPoints'] = Decoder.prototype.GetAttributeUInt8ForAllPoints = function(pc, pa, out_values) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (pa && typeof pa === 'object') pa = pa.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetAttributeUInt8ForAllPoints_3(self, pc, pa, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeInt16ForAllPoints'] = Decoder.prototype.GetAttributeInt16ForAllPoints = function(pc, pa, out_values) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (pa && typeof pa === 'object') pa = pa.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetAttributeInt16ForAllPoints_3(self, pc, pa, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeUInt16ForAllPoints'] = Decoder.prototype.GetAttributeUInt16ForAllPoints = function(pc, pa, out_values) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (pa && typeof pa === 'object') pa = pa.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetAttributeUInt16ForAllPoints_3(self, pc, pa, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeInt32ForAllPoints'] = Decoder.prototype.GetAttributeInt32ForAllPoints = function(pc, pa, out_values) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (pa && typeof pa === 'object') pa = pa.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetAttributeInt32ForAllPoints_3(self, pc, pa, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeUInt32ForAllPoints'] = Decoder.prototype.GetAttributeUInt32ForAllPoints = function(pc, pa, out_values) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (pa && typeof pa === 'object') pa = pa.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetAttributeUInt32ForAllPoints_3(self, pc, pa, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetAttributeDataArrayForAllPoints'] = Decoder.prototype.GetAttributeDataArrayForAllPoints = function(pc, pa, data_type, out_size, out_values) {
  var self = this.ptr;
  if (pc && typeof pc === 'object') pc = pc.ptr;
  if (pa && typeof pa === 'object') pa = pa.ptr;
  if (data_type && typeof data_type === 'object') data_type = data_type.ptr;
  if (out_size && typeof out_size === 'object') out_size = out_size.ptr;
  if (out_values && typeof out_values === 'object') out_values = out_values.ptr;
  return !!(_emscripten_bind_Decoder_GetAttributeDataArrayForAllPoints_5(self, pc, pa, data_type, out_size, out_values));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['SkipAttributeTransform'] = Decoder.prototype.SkipAttributeTransform = function(att_type) {
  var self = this.ptr;
  if (att_type && typeof att_type === 'object') att_type = att_type.ptr;
  _emscripten_bind_Decoder_SkipAttributeTransform_1(self, att_type);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['GetEncodedGeometryType_Deprecated'] = Decoder.prototype.GetEncodedGeometryType_Deprecated = function(in_buffer) {
  var self = this.ptr;
  if (in_buffer && typeof in_buffer === 'object') in_buffer = in_buffer.ptr;
  return _emscripten_bind_Decoder_GetEncodedGeometryType_Deprecated_1(self, in_buffer);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['DecodeBufferToPointCloud'] = Decoder.prototype.DecodeBufferToPointCloud = function(in_buffer, out_point_cloud) {
  var self = this.ptr;
  if (in_buffer && typeof in_buffer === 'object') in_buffer = in_buffer.ptr;
  if (out_point_cloud && typeof out_point_cloud === 'object') out_point_cloud = out_point_cloud.ptr;
  return wrapPointer(_emscripten_bind_Decoder_DecodeBufferToPointCloud_2(self, in_buffer, out_point_cloud), Status);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['DecodeBufferToMesh'] = Decoder.prototype.DecodeBufferToMesh = function(in_buffer, out_mesh) {
  var self = this.ptr;
  if (in_buffer && typeof in_buffer === 'object') in_buffer = in_buffer.ptr;
  if (out_mesh && typeof out_mesh === 'object') out_mesh = out_mesh.ptr;
  return wrapPointer(_emscripten_bind_Decoder_DecodeBufferToMesh_2(self, in_buffer, out_mesh), Status);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
Decoder.prototype['__destroy__'] = Decoder.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_Decoder___destroy___0(self);
};

(function() {
  function setupEnums() {
    
// $draco_AttributeTransformType

    Module['ATTRIBUTE_INVALID_TRANSFORM'] = _emscripten_enum_draco_AttributeTransformType_ATTRIBUTE_INVALID_TRANSFORM();

    Module['ATTRIBUTE_NO_TRANSFORM'] = _emscripten_enum_draco_AttributeTransformType_ATTRIBUTE_NO_TRANSFORM();

    Module['ATTRIBUTE_QUANTIZATION_TRANSFORM'] = _emscripten_enum_draco_AttributeTransformType_ATTRIBUTE_QUANTIZATION_TRANSFORM();

    Module['ATTRIBUTE_OCTAHEDRON_TRANSFORM'] = _emscripten_enum_draco_AttributeTransformType_ATTRIBUTE_OCTAHEDRON_TRANSFORM();

    
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

    
// $draco_DataType

    Module['DT_INVALID'] = _emscripten_enum_draco_DataType_DT_INVALID();

    Module['DT_INT8'] = _emscripten_enum_draco_DataType_DT_INT8();

    Module['DT_UINT8'] = _emscripten_enum_draco_DataType_DT_UINT8();

    Module['DT_INT16'] = _emscripten_enum_draco_DataType_DT_INT16();

    Module['DT_UINT16'] = _emscripten_enum_draco_DataType_DT_UINT16();

    Module['DT_INT32'] = _emscripten_enum_draco_DataType_DT_INT32();

    Module['DT_UINT32'] = _emscripten_enum_draco_DataType_DT_UINT32();

    Module['DT_INT64'] = _emscripten_enum_draco_DataType_DT_INT64();

    Module['DT_UINT64'] = _emscripten_enum_draco_DataType_DT_UINT64();

    Module['DT_FLOAT32'] = _emscripten_enum_draco_DataType_DT_FLOAT32();

    Module['DT_FLOAT64'] = _emscripten_enum_draco_DataType_DT_FLOAT64();

    Module['DT_BOOL'] = _emscripten_enum_draco_DataType_DT_BOOL();

    Module['DT_TYPES_COUNT'] = _emscripten_enum_draco_DataType_DT_TYPES_COUNT();

    
// $draco_StatusCode

    Module['OK'] = _emscripten_enum_draco_StatusCode_OK();

    Module['DRACO_ERROR'] = _emscripten_enum_draco_StatusCode_DRACO_ERROR();

    Module['IO_ERROR'] = _emscripten_enum_draco_StatusCode_IO_ERROR();

    Module['INVALID_PARAMETER'] = _emscripten_enum_draco_StatusCode_INVALID_PARAMETER();

    Module['UNSUPPORTED_VERSION'] = _emscripten_enum_draco_StatusCode_UNSUPPORTED_VERSION();

    Module['UNKNOWN_VERSION'] = _emscripten_enum_draco_StatusCode_UNKNOWN_VERSION();

  }
  if (runtimeInitialized) setupEnums();
  else addOnInit(setupEnums);
})();
