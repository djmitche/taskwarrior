use ffizz_string::FzString;
use taskchampion::chrono::prelude::*;
use taskchampion::utc_timestamp;

pub(crate) fn err_to_fzstring(e: impl std::string::ToString) -> FzString<'static> {
    FzString::from(e.to_string())
}

/// An implementation of Vec::into_raw_parts, which is still unstable.  Returns ptr, len, cap.
pub(crate) fn vec_into_raw_parts<T>(vec: Vec<T>) -> (*mut T, usize, usize) {
    // emulate Vec::into_raw_parts():
    // - disable dropping the Vec with ManuallyDrop
    // - extract ptr, len, and capacity using those methods
    let mut vec = std::mem::ManuallyDrop::new(vec);
    (vec.as_mut_ptr(), vec.len(), vec.capacity())
}

/// Convert a `time_t` value from C into a `chrono::DateTime<Utc>`.  Returns None if the time_t is
/// zero or invalid.
pub(crate) fn time_t_to_chrono(time: libc::time_t) -> Option<DateTime<Utc>> {
    if time != 0 {
        return Some(utc_timestamp(time));
    }
    None
}

/// Convert a `chrono::DateTime<Utc>` into a `time_t` for passing back to C.
pub(crate) fn chrono_to_time_t(ts: Option<DateTime<Utc>>) -> libc::time_t {
    ts.map(|ts| ts.timestamp() as libc::time_t)
        .unwrap_or(0 as libc::time_t)
}
