#include "ResourceMetadataManager.h"
#include "core/util/Assert.h"
#include <iostream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ResourceMetadataManager::ResourceMetadataManager(){
	// initialize writeview
	pthread_spin_init(&m_spinlock, 0);

	writeview = NULL;
}
void ResourceMetadataManager::commitClusterMetadata(ClusterResourceMetadata_Readview * newReadview){
    pthread_spin_lock(&m_spinlock);
    // set readview pointer to the new copy of writeview
    metadata_readView.reset(newReadview);
    pthread_spin_unlock(&m_spinlock);
}

void ResourceMetadataManager::commitClusterMetadata(){
    commitClusterMetadata(this->writeview->getNewReadview());
}
void ResourceMetadataManager::getClusterReadView(boost::shared_ptr<const ClusterResourceMetadata_Readview> & clusterReadview) const{
	// We need the lock it to prevent the following two operations from happening at the same time.
	// One reader is doing reader = readview, which is reading the readview.
	// At the same time, we can call merge(), in which we can have "readview=writeview", which is modifying the read view.
	pthread_spin_lock(&m_spinlock);
	clusterReadview = metadata_readView;
	pthread_spin_unlock(&m_spinlock);
}
void ResourceMetadataManager::setWriteview(Cluster_Writeview * newWriteview){
	if(newWriteview == NULL){
		ASSERT(false);
		return;
	}
	if(writeview != NULL){
		delete writeview;
	}
	writeview = newWriteview;
}

unsigned ResourceMetadataManager::applyAndCommit(MetadataChange * metadataChange, Cluster_Writeview * writeview){
	// apply change on writeview and commit
	if(metadataChange == NULL){
		ASSERT(false);
		return 0;
	}
	metadataChange->doChange(writeview);
	this->commitClusterMetadata(writeview->getNewReadview());
	return writeview->versionId - 1;
}

unsigned ResourceMetadataManager::applyAndCommit(MetadataChange * metadataChange){
	return applyAndCommit(metadataChange, this->writeview);
}

Cluster_Writeview * ResourceMetadataManager::getClusterWriteview() const{
	return writeview;
}

void ResourceMetadataManager::print(){
	cout << "**************************************************************************************************" << endl;
	if(writeview == NULL){
		cout << "Write view is not available." << endl;
		cout << "**************************************************************************************************" << endl;
	}else{
		cout << "Writeview : " << endl;
		cout << "**************************************************************************************************" << endl;
		writeview->print();
	}
	cout << "**************************************************************************************************" << endl;
	if(! metadata_readView){
		cout << "Read view is not available." << endl;
		cout << "**************************************************************************************************" << endl;
	}else{
		cout << "Readview : " << endl;
		cout << "**************************************************************************************************" << endl;
		metadata_readView->print();
	}
	cout << "**************************************************************************************************" << endl;
}

}
}
