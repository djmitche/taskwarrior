use crate::traits::*;
use ffizz_string::FzString;

#[ffizz_header::item]
#[ffizz(order = 200)]
/// ***** TCString *****
///
/// TCString supports passing strings into and out of the TaskChampion API.  It is an opaque
/// type that can only be accessed via the provided `tc_string_…` functions.
///
/// # Null Stirngs
///
/// This value can contain either a string or a special "Null" variant indicating there is no
/// string.  When functions take a `TCString*` as an argument, the NULL pointer is treated as
/// the Null variant.  Note that the Null variant is not necessarily represented as the zero value
/// of the struct.
///
/// Note that taskchampion functions require that strings given as arguments are not Null, unless
/// documented otherwise.
///
/// Null strings must still be freed with `tc_string_free`.
///
/// # Rust Strings and C Strings
///
/// A Rust string can contain embedded NUL characters, while C considers such a character to mark
/// the end of a string.  Strings containing embedded NULs cannot be represented as a "C string"
/// and must be accessed using `tc_string_content_and_len` and `tc_string_clone_with_len`.  In
/// general, these two functions should be used for handling arbitrary data, while more convenient
/// forms may be used where embedded NUL characters are impossible, such as in static strings.
///
/// # UTF-8
///
/// TaskChampion expects all strings to be valid UTF-8. `tc_string_…` functions will fail if given
/// a `*TCString` containing invalid UTF-8.
///
/// # Safety
///
/// A TCString must always be initialized before it is passed as an argument.  Functions
/// returning a `TCString` return an initialized value.
///
/// Each initialized TCString must be freed, either by calling tc_string_free or by passing the
/// string to a function which takes ownership of the string.  Unless specified otherwise,
/// TaskChampion functions take ownership of a `TCString` when it is given as a function argument,
/// and the caller must not use or free TCStrings after passing them to such API functions.
///
/// For a given TCString value, API functions must not be called concurrently.  This includes
/// "read only" functions such as tc_string_content.
///
/// ```c
/// typedef struct TCString {
///     uint64_t __reserved[4];
/// } TCString;
/// ```
pub use ffizz_string::fz_string_t as TCString;

// Temporary until TC uses ffizz-passby.
impl PassByValue for TCString {
    type RustType = FzString<'static>;

    unsafe fn from_ctype(self) -> Self::RustType {
        // SAFETY: self is a valid value
        unsafe { FzString::take(self) }
    }

    fn as_ctype(rself: Self::RustType) -> Self {
        // SAFETY: rself will eventually be freed
        unsafe { rself.return_val() }
    }
}

#[ffizz_header::item]
#[ffizz(order = 210)]
/// ***** TCStringList *****
///
/// TCStringList represents a list of strings.
///
/// The content of this struct must be treated as read-only.
///
/// ```c
/// typedef struct TCStringList {
///   // number of strings in items
///   size_t len;
///   // reserved
///   size_t _u1;
///   // TCStringList representing each string. These remain owned by the TCStringList instance and will
///   // be freed by tc_string_list_free.  This pointer is never NULL for a valid TCStringList, and the
///   // *TCStringList at indexes 0..len-1 are not NULL.
///   struct TCString *items;
/// } TCStringList;
/// ```
#[repr(C)]
pub struct TCStringList {
    /// number of strings in items
    len: libc::size_t,

    /// total size of items (internal use only)
    capacity: libc::size_t,

    /// Array of strings. These remain owned by the TCStringList instance and will be freed by
    /// tc_string_list_free.  This pointer is never NULL for a valid TCStringList, and the
    /// *TCStringList at indexes 0..len-1 are not NULL.
    items: *mut TCString,
}

impl CList for TCStringList {
    type Element = TCString;

    unsafe fn from_raw_parts(items: *mut Self::Element, len: usize, cap: usize) -> Self {
        TCStringList {
            len,
            capacity: cap,
            items,
        }
    }

    fn slice(&mut self) -> &mut [Self::Element] {
        // SAFETY:
        //  - because we have &mut self, we have read/write access to items[0..len]
        //  - all items are properly initialized Element's
        //  - return value lifetime is equal to &mmut self's, so access is exclusive
        //  - items and len came from Vec, so total size is < isize::MAX
        unsafe { std::slice::from_raw_parts_mut(self.items, self.len) }
    }

    fn into_raw_parts(self) -> (*mut Self::Element, usize, usize) {
        (self.items, self.len, self.capacity)
    }
}

ffizz_header::snippet! {
#[ffizz(order = 201, name="tc_string_borrow")]
/// Create a new TCString containing a pointer to the given C string.
///
/// # Safety
///
/// The C string must remain valid and unchanged until after the TCString is freed.  It's
/// typically easiest to ensure this by using a static string.
///
/// ```c
/// EXTERN_C TCString tc_string_borrow(const char *);
/// ```
}
ffizz_string::reexport!(fz_string_borrow as tc_string_borrow);

ffizz_header::snippet! {
#[ffizz(order = 201, name="tc_string_null")]
/// Create a new, null `TCString`.  Note that this is _not_ the zero value of `TCString`.
///
/// ```c
/// EXTERN_C TCString tc_string_null();
/// ```
}
ffizz_string::reexport!(fz_string_null as tc_string_null);

ffizz_header::snippet! {
#[ffizz(order = 201, name="tc_string_clone")]
/// Create a new `TCString` by cloning the content of the given C string.  The resulting `TCString`
/// is independent of the given string.
///
/// # Safety
///
/// The given pointer must not be NULL.
///
/// ```c
/// EXTERN_C TCString tc_string_clone(const char *);
/// ```
}
ffizz_string::reexport!(fz_string_clone as tc_string_clone);

ffizz_header::snippet! {
#[ffizz(order = 201, name="tc_string_clone_with_len")]
/// Create a new `TCString` containing the given string with the given length. This allows creation
/// of strings containing embedded NUL characters.  As with `fz_string_clone`, the resulting
/// `TCString` is independent of the passed buffer.
///
/// The given length should _not_ include any NUL terminator.  The given length must be less than
/// half the maximum value of size_t.
///
/// # Safety
///
/// The given pointer must not be NULL.
///
/// ```c
/// EXTERN_C TCString tc_string_clone_with_len(const char *ptr, size_t len);
/// ```
}
ffizz_string::reexport!(fz_string_clone_with_len as tc_string_clone_with_len);

ffizz_header::snippet! {
#[ffizz(order = 201, name="tc_string_content")]
/// Get the content of the string as a regular C string.
///
/// A string contianing NUL bytes will result in a NULL return value.  In general, prefer
/// `fz_string_content_with_len` except when it's certain that the string is NUL-free.
///
/// The Null variant also results in a NULL return value.
///
/// This function takes the `TCString` by pointer because it may be modified in-place to add a NUL
/// terminator.  The pointer must not be NULL.
///
/// This function does _not_ take ownership of the TCString.
///
/// # Safety
///
/// The returned string is "borrowed" and remains valid only until the `TCString` is freed or
/// passed to any other API function.
///
/// ```c
/// EXTERN_C const char *tc_string_content(const struct TCString *tcstring);
/// ```
}
ffizz_string::reexport!(fz_string_content as tc_string_content);

ffizz_header::snippet! {
#[ffizz(order = 201, name="tc_string_content_with_len")]
/// Get the content of the string as a pointer and length.
///
/// This function can return any string, even one including NUL bytes or invalid UTF-8.
/// If the TCString is the Null variant, this returns NULL and the length is set to zero.
///
/// This function does _not_ take ownership of the TCString.
///
/// # Safety
///
/// The returned string is "borrowed" and remains valid only until the `TCString` is freed or
/// passed to any other API function.
///
/// This function takes the TCString by pointer because it may be modified in-place to add a NUL
/// terminator.  The pointer must not be NULL.
///
/// ```c
/// EXTERN_C const char *tc_string_content_with_len(const struct TCString *, size_t *len_out);
/// ```
}
ffizz_string::reexport!(fz_string_content_with_len as tc_string_content_with_len);

ffizz_header::snippet! {
#[ffizz(order = 201, name="tc_string_is_null")]
/// Determine whether the given `TCString` is a Null variant.
///
/// ```c
/// EXTERN_C bool tc_string_is_null(TCString *);
/// ```
}
ffizz_string::reexport!(fz_string_is_null as tc_string_is_null);

ffizz_header::snippet! {
#[ffizz(order = 201, name="tc_string_free")]
/// Free a `TCString`.
///
/// # Safety
///
/// The string must not be used after this function returns, and must not be freed more than once.
/// It is safe to free Null-variant strings.
///
/// ```c
/// EXTERN_C void tc_string_free(TCString *);
/// ```
}
ffizz_string::reexport!(fz_string_free as tc_string_free);

#[ffizz_header::item]
#[ffizz(order = 211)]
/// Free a TCStringList instance.  The instance, and all TCStringList it contains, must not be used after
/// this call.
///
/// When this call returns, the `items` pointer will be NULL, signalling an invalid TCStringList.
///
/// ```c
/// EXTERN_C void tc_string_list_free(struct TCStringList *tcstrings);
/// ```
#[no_mangle]
pub unsafe extern "C" fn tc_string_list_free(tcstrings: *mut TCStringList) {
    // SAFETY:
    //  - tcstrings is not NULL and points to a valid TCStringList (caller is not allowed to
    //    modify the list)
    //  - caller promises not to use the value after return
    unsafe { drop_value_list(tcstrings) };
}

#[cfg(test)]
mod test {
    use super::*;
    use pretty_assertions::assert_eq;
    use std::ffi::{CStr, CString};

    #[test]
    fn empty_list_has_non_null_pointer() {
        let tcstrings = unsafe { TCStringList::return_val(Vec::new()) };
        assert!(!tcstrings.items.is_null());
        assert_eq!(tcstrings.len, 0);
        assert_eq!(tcstrings.capacity, 0);
    }

    #[test]
    fn free_sets_null_pointer() {
        let mut tcstrings = unsafe { TCStringList::return_val(Vec::new()) };
        // SAFETY: testing expected behavior
        unsafe { tc_string_list_free(&mut tcstrings) };
        assert!(tcstrings.items.is_null());
        assert_eq!(tcstrings.len, 0);
        assert_eq!(tcstrings.capacity, 0);
    }

    const INVALID_UTF8: &[u8] = b"abc\xf0\x28\x8c\x28";

    fn make_cstring() -> FzString<'static> {
        FzString::CString(CString::new("a string").unwrap())
    }

    fn make_cstr() -> FzString<'static> {
        let cstr = CStr::from_bytes_with_nul(b"a string\0").unwrap();
        FzString::CStr(cstr)
    }

    fn make_string() -> FzString<'static> {
        FzString::String("a string".into())
    }

    fn make_string_with_nul() -> FzString<'static> {
        FzString::String("a \0 nul!".into())
    }

    fn make_invalid_bytes() -> FzString<'static> {
        FzString::Bytes(INVALID_UTF8.to_vec())
    }

    fn make_bytes() -> FzString<'static> {
        FzString::Bytes(b"bytes".to_vec())
    }

    #[test]
    fn cstring_as_str() {
        assert_eq!(make_cstring().as_str().unwrap().unwrap(), "a string");
    }

    #[test]
    fn cstr_as_str() {
        assert_eq!(make_cstr().as_str().unwrap().unwrap(), "a string");
    }

    #[test]
    fn string_as_str() {
        assert_eq!(make_string().as_str().unwrap().unwrap(), "a string");
    }

    #[test]
    fn string_with_nul_as_str() {
        assert_eq!(
            make_string_with_nul().as_str().unwrap().unwrap(),
            "a \0 nul!"
        );
    }

    #[test]
    fn invalid_bytes_as_str() {
        assert!(make_invalid_bytes().as_str().is_err());
    }

    #[test]
    fn valid_bytes_as_str() {
        assert_eq!(make_bytes().as_str().unwrap().unwrap(), "bytes");
    }

    #[test]
    fn cstring_as_bytes() {
        assert_eq!(make_cstring().as_bytes().unwrap(), b"a string");
    }

    #[test]
    fn cstr_as_bytes() {
        assert_eq!(make_cstr().as_bytes().unwrap(), b"a string");
    }

    #[test]
    fn string_as_bytes() {
        assert_eq!(make_string().as_bytes().unwrap(), b"a string");
    }

    #[test]
    fn string_with_nul_as_bytes() {
        assert_eq!(make_string_with_nul().as_bytes().unwrap(), b"a \0 nul!");
    }

    #[test]
    fn invalid_bytes_as_bytes() {
        assert_eq!(make_invalid_bytes().as_bytes().unwrap(), INVALID_UTF8);
    }
}
