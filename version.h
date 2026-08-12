#define _HIGHWIRE_MAJOR_     0
#define _HIGHWIRE_MINOR_     3
#define _HIGHWIRE_REVISION_  5
#define _HIGHWIRE_BETATAG_   "\341" "7"
#define _HIGHWIRE_VERSION_   "0.3.5"

/* This fork's own revision, deliberately separate from the HighWire version
 * above: upstream's numbering is theirs and should never read as ours.  Not
 * folded into _HIGHWIRE_FULLNAME_ either, which is interpolated into the
 * User-Agent -- we are a build of HighWire, and say so on the wire.
*/
#define _LOWWIRE_REV_        "r1"
/* Short enough for the About alert: form_alert wraps well before hwUi_box's
 * documented 40 characters, and the path was being cut off on real TOS. */
#define _LOWWIRE_URL_        "neilrackett.com"
