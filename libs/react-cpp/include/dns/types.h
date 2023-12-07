/**
 *  Types.h
 *
 *  Types and callback that are used by the resolver
 *
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React { namespace Dns {

/**
 *  Forward declarations
 */
class MxResult;

/**
 *  Types
 */
using IpResult          =   std::set<IpRecord>;
 
/**
 *  Callbacks
 */
using IpCallback        =   std::function<void(IpResult &&ips, const char *error)>;
using MxCallback        =   std::function<void(MxResult &&mx, const char *error)>;

/**
 *  End namespace
 */
}}

