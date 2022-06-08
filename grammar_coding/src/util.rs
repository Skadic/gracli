
#[repr(C)]
/// Represents a vector with a pointer and the number of elements.
/// Note that the allocated memory is not deleted automatically!
pub struct RawVec<T> {
    pub ptr: *mut T,
    pub len: usize,
}
