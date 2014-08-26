/*
 * AccessControl.h
 *
 *  Created on: Aug 18, 2014
 *      Author: srch2
 */

#ifndef __CORE_OPERATION_ACCESSCONTROL_H__
#define __CORE_OPERATION_ACCESSCONTROL_H__

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "record/SchemaInternal.h"

namespace srch2 {
namespace instantsearch {

typedef boost::shared_mutex AttributeAclLock;
typedef boost::unique_lock< AttributeAclLock > WriterLock;
typedef boost::shared_lock< AttributeAclLock > ReaderLock;
using namespace std;
typedef pair< boost::shared_ptr<vector<unsigned> >, boost::shared_ptr<vector<unsigned> > > PairOfAttrsListSharedPtr;
typedef map<string, PairOfAttrsListSharedPtr>::iterator AclMapIter;
typedef map<string, PairOfAttrsListSharedPtr>::const_iterator AclMapConstIter;

enum AclActionType {
	ACL_INSERT,
	ACL_DELETE,
	ACL_APPEND,
};
class AttributeAccessControl {
public:
	AttributeAccessControl(const SchemaInternal *schema) {
		this->schema = schema;
	}
	// ----------------------------
	// read operations
	// ----------------------------

	// Get accessible searchable attributes for a given acl role name.
	void fetchSearchableAttrsAcl(string aclRoleValue, boost::shared_ptr<vector<unsigned> >& attrList) const;

	// Get accessible refining attributes for a given acl role name.
	void fetchRefiningAttrsAcl(string aclRoleValue, boost::shared_ptr<vector<unsigned> >& attrList) const;

	// ----------------------------
	// write operations
	// ----------------------------

	// Bulk load ACL for first time.
	// Thread unsafe. Should be called only form main thread during initial load.
	void bulkLoadAcl(const string& aclLoadFileName) const;

	// process ReST HTTP API
	bool processHTTPAclRequest(const string& fields, const string& roleValues, AclActionType action) const;

	// add new acl for a role
	void setAcl(string aclRoleValue, vector<unsigned>& searchableAttrIdsList,
			vector<unsigned>& refiningAttrIdsList);

	// append to existing acl for a role. If role is not found then it is created.
	void appendToAcl(string aclRoleValue, const vector<unsigned>& searchableAttrIdsList,
			const vector<unsigned>& refiningAttrIdsList);

	// delete the attributes from existing acl for a role. If role is not found then it is ignored.
	void deleteFromAcl(string aclRoleValue, const vector<unsigned>& searchableAttrIdsList,
			const vector<unsigned>& refiningAttrIdsList);

	virtual ~AttributeAccessControl() {
	}

	void toString(stringstream& ss) const;

private:
	mutable AttributeAclLock attrAclLock;
	map<string, PairOfAttrsListSharedPtr > attributeAclMap;
	const SchemaInternal *schema;

	// private Helper functions

	//convert attribute names to attribute ids
	void covertFieldNamesToSortedFieldIds(vector<string>& fieldTokens,
			vector<unsigned>& searchableAttrIdsList, vector<unsigned>& refiningAttrIdsList) const;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & attributeAclMap;
//    	stringstream ss;
//    	toString(ss);
//    	cout << ss.str() << endl;
    }
};

} /* namespace instantsearch */
} /* namespace srch2 */
#endif /* __CORE_OPERATION_ACCESSCONTROL_H__ */
