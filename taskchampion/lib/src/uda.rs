use crate::traits::*;
use crate::types::*;
use ffizz_passby::OpaqueStruct;
use ffizz_string::FzString;

#[ffizz_header::item]
#[ffizz(order = 500)]
/// ***** TCUda *****
///
/// TCUda contains the details of a UDA.
///
/// ```c
/// typedef struct TCUda {
///   // Namespace of the UDA.  For legacy UDAs, this may be Null.
///   struct TCString ns;
///   // UDA key.  Must not be Null.
///   struct TCString key;
///   // Content of the UDA.  Must not be Null.
///   struct TCString value;
/// } TCUda;
/// ```
#[repr(C)]
pub struct TCUda {
    /// Namespace of the UDA.  For legacy UDAs, this may be Null.
    pub ns: TCString,
    /// UDA key.  Must not be Null.
    pub key: TCString,
    /// Content of the UDA.  Must not be Null.
    pub value: TCString,
}

pub(crate) struct Uda {
    pub ns: FzString<'static>,
    pub key: FzString<'static>,
    pub value: FzString<'static>,
}

impl PassByValue for TCUda {
    type RustType = Uda;

    unsafe fn from_ctype(self) -> Self::RustType {
        Uda {
            // SAFETY:
            //  - field is a valid TCString (per type docstring)
            //  - field is owned by self, so .. well, this is unsafe TODO
            ns: unsafe { FzString::take(self.ns) },
            // SAFETY: (same)
            key: unsafe { FzString::take(self.key) },
            // SAFETY: (same)
            value: unsafe { FzString::take(self.value) },
        }
    }

    fn as_ctype(uda: Uda) -> Self {
        TCUda {
            // SAFETY: caller assumes ownership of this value
            ns: unsafe { uda.ns.return_val() },
            // SAFETY: caller assumes ownership of this value
            key: unsafe { uda.key.return_val() },
            // SAFETY: caller assumes ownership of this value
            value: unsafe { uda.value.return_val() },
        }
    }
}

// TODO: could derive if FzString had a default
impl Default for TCUda {
    fn default() -> Self {
        TCUda {
            // SAFETY: ownership of Null variant passes to annotation
            ns: unsafe { FzString::Null.return_val() },
            // SAFETY: same
            key: unsafe { FzString::Null.return_val() },
            // SAFETY: same
            value: unsafe { FzString::Null.return_val() },
        }
    }
}

#[ffizz_header::item]
#[ffizz(order = 510)]
/// ***** TCUdaList *****
///
/// TCUdaList represents a list of UDAs.
///
/// The content of this struct must be treated as read-only.
///
/// ```c
/// typedef struct TCUdaList {
///   // number of UDAs in items
///   size_t len;
///   // reserved
///   size_t _u1;
///   // Array of UDAs. These remain owned by the TCUdaList instance and will be freed by
///   // tc_uda_list_free.  This pointer is never NULL for a valid TCUdaList.
///   struct TCUda *items;
/// } TCUdaList;
/// ```
#[repr(C)]
pub struct TCUdaList {
    /// number of UDAs in items
    len: libc::size_t,

    /// total size of items (internal use only)
    _capacity: libc::size_t,

    /// Array of UDAs. These remain owned by the TCUdaList instance and will be freed by
    /// tc_uda_list_free.  This pointer is never NULL for a valid TCUdaList.
    items: *mut TCUda,
}

impl CList for TCUdaList {
    type Element = TCUda;

    unsafe fn from_raw_parts(items: *mut Self::Element, len: usize, cap: usize) -> Self {
        TCUdaList {
            len,
            _capacity: cap,
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
        (self.items, self.len, self._capacity)
    }
}

#[ffizz_header::item]
#[ffizz(order = 501)]
/// Free a TCUda instance.  The instance, and the TCStrings it contains, must not be used
/// after this call.
///
/// ```c
/// EXTERN_C void tc_uda_free(struct TCUda *tcuda);
/// ```
#[no_mangle]
pub unsafe extern "C" fn tc_uda_free(tcuda: *mut TCUda) {
    debug_assert!(!tcuda.is_null());
    // SAFETY:
    //  - *tcuda is a valid TCUda (caller promises to treat it as read-only)
    let uda = unsafe { TCUda::take_val_from_arg(tcuda, TCUda::default()) };
    drop(uda);
}

#[ffizz_header::item]
#[ffizz(order = 511)]
/// Free a TCUdaList instance.  The instance, and all TCUdas it contains, must not be used after
/// this call.
///
/// When this call returns, the `items` pointer will be NULL, signalling an invalid TCUdaList.
///
/// ```c
/// EXTERN_C void tc_uda_list_free(struct TCUdaList *tcudas);
/// ```
#[no_mangle]
pub unsafe extern "C" fn tc_uda_list_free(tcudas: *mut TCUdaList) {
    // SAFETY:
    //  - tcudas is not NULL and points to a valid TCUdaList (caller is not allowed to
    //    modify the list)
    //  - caller promises not to use the value after return
    unsafe { drop_value_list(tcudas) }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn empty_list_has_non_null_pointer() {
        let tcudas = unsafe { TCUdaList::return_val(Vec::new()) };
        assert!(!tcudas.items.is_null());
        assert_eq!(tcudas.len, 0);
        assert_eq!(tcudas._capacity, 0);
    }

    #[test]
    fn free_sets_null_pointer() {
        let mut tcudas = unsafe { TCUdaList::return_val(Vec::new()) };
        // SAFETY: testing expected behavior
        unsafe { tc_uda_list_free(&mut tcudas) };
        assert!(tcudas.items.is_null());
        assert_eq!(tcudas.len, 0);
        assert_eq!(tcudas._capacity, 0);
    }
}
