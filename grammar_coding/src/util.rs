use std::slice;


#[repr(C)]
/// Represents a vector with a pointer and the number of elements.
/// Note that the allocated memory is not deleted automatically!
pub struct RawVec<T> {
    pub ptr: *mut T,
    pub len: usize,
}

impl<T> RawVec<T> {
    pub fn iter(&self) -> std::slice::Iter<T> {
        AsRef::<[T]>::as_ref(self).iter()
    } 

    pub unsafe fn to_vec(self) {
        let _ = Vec::from_raw_parts(self.ptr, self.len, self.len);
    }
}

impl<T> AsRef<[T]> for RawVec<T> {
    fn as_ref(&self) -> &[T] {
        unsafe { slice::from_raw_parts(self.ptr, self.len) }
    }
}
