#pragma once

// TODO(you): Reliability on top of UDP lab session 
// TODO(you): Reliability on top of UDP lab session

#include <list>

class DeliveryManager;

class DeliveryDelegate {

public:

	virtual void OnDeliverySuccess(DeliveryManager* deliveryManager) = 0;
	virtual void OnDeliveryFailure(DeliveryManager* deliveryManager) = 0;

};

struct Delivery {

	uint32 sequenceNumber = 0;
	double dispatchTime = 0.0;
	DeliveryDelegate* delegate = nullptr;

};

class DeliveryManager {

public:

	// For senders to write a nwe seq. number into a packet 
	Delivery* WriteSequenceNumber(OutputMemoryStream& packet);

	// For Receivers to proccess the seq. number from an incoming packet 
	bool ProcessSequenceNumber(const InputMemoryStream& packet);

	// For receivers to write acknowladged seq. numbers into a packet 
	bool hasSequenceNumbersPendingAck();
	void WriteSequenceNumbersPendingAck(OutputMemoryStream& packet);

	// For senders to process acknowladged seq. numbers from a packet 
	void ProcessAckdSequenceNumbers(const InputMemoryStream& packet);
	void ProcessTimedOutPackets();

	void Clear();

private:

	// Senders
	uint32 nextOutSequenceNumber = 0;
	std::list<Delivery*> pendingDeliveries;

	// Recievers
	uint32 nextExpectedSequenceNumber = 0;
	std::list<uint32> pendingAcks;
};