/*
 * @file J2735MessageFactory.hpp
 *
 *  Created on: May 17, 2016
 *      @author: Gregory M. Baumgardner
 */

#ifndef TMX_J2735_MESSAGES_J2735MESSAGEFACTORY_HPP_
#define TMX_J2735_MESSAGES_J2735MESSAGEFACTORY_HPP_

#include <iomanip>
#include <map>
#include <memory>

#include <tmx/apimessages/TmxEventLog.hpp>

// Need to include all the types
#include <tmx/j2735_messages/BasicSafetyMessage.hpp>
#include <tmx/j2735_messages/BasicSafetyMessageVerbose.hpp>
#include <tmx/j2735_messages/CommonSafetyRequestMessage.hpp>
#include <tmx/j2735_messages/EmergencyVehicleAlertMessage.hpp>
#include <tmx/j2735_messages/IntersectionCollisionMessage.hpp>
#include <tmx/j2735_messages/MapDataMessage.hpp>
#include <tmx/j2735_messages/NmeaMessage.hpp>
#include <tmx/j2735_messages/PersonalMobilityMessage.hpp>
#include <tmx/j2735_messages/PersonalSafetyMessage.hpp>
#include <tmx/j2735_messages/ProbeDataManagementMessage.hpp>
#include <tmx/j2735_messages/ProbeVehicleDataMessage.hpp>
#include <tmx/j2735_messages/RoadSideAlertMessage.hpp>
#include <tmx/j2735_messages/RtcmMessage.hpp>
#include <tmx/j2735_messages/SignalRequestMessage.hpp>
#include <tmx/j2735_messages/SignalStatusMessage.hpp>
#include <tmx/j2735_messages/SpatMessage.hpp>
#include <tmx/j2735_messages/TravelerInformationMessage.hpp>

namespace tmx {
namespace messages {

struct ENDOFLIST {};

/// Variadic template for the message type list
template <class... T> struct message_type_list {};
using message_types = message_type_list<
		BsmMessage,
		BsmvMessage,
		CsrMessage,
		EvaMessage,
		IntersectionCollisionMessage,
		MapDataMessage,
		NmeaMessage,
		PdmMessage,
		PmmMessage,
		PsmMessage,
		PvdMessage,
		RsaMessage,
		RtcmMessage,
		SrmMessage,
		SsmMessage,
		SpatMessage,
		TimMessage
>;

/// Base allocator type
struct message_allocator
{
	virtual ~message_allocator() {}

	/// Get the J2735 message ID for the message
	virtual int getMessageID() { return 0; };
	/// Get the message type name for the message
	virtual const char *getMessageType() { return ""; };
	/// Create a new instance of the J2735 message
	virtual TmxJ2735EncodedMessageBase *allocate() { return NULL; };
};

/// Templated allocator implementation for the messages
template <typename MessageType>
struct message_allocator_impl: public message_allocator
{
	typedef MessageType type;

	virtual ~message_allocator_impl() {}
	/// @see TmxJ2735Message<>::get_default_msgId()
	inline int getMessageID() { return type::get_default_msgId(); }
	/// @see TmxJ2735Message<>::MessageSubType
	inline const char *getMessageType() { return type::MessageSubType; }
	/// @see TmxJ2735Message<>()
	inline TmxJ2735EncodedMessageBase *allocate() { return new TmxJ2735EncodedMessage<type>(); }
};

/**
 * A factory class to create J2735 messages.  The trouble with the various messages is that the programmer must
 * know specifics about what type the message is in order to correctly send the message.  Therefore, this class
 * encompasses all the complexity of creating a new J2735 message type from either the type name or the message
 * ID.
 */
class J2735MessageFactory
{
private:
	using int_map = std::map<int, message_allocator *>;
	using str_map = std::map<std::string, message_allocator *>;

	int_map &byInt;
	str_map &byStr;

	/**
	 * @return The only static instance of the map by message ID
	 */
	static int_map &get_int_map()
	{
		static int_map theMap;
		return theMap;
	}

	/**
	 * @return The only static instance of the map by message type
	 */
	static str_map &get_str_map()
	{
		static str_map theMap;
		return theMap;
	}

	/**
	 * A template function to build a new allocator for the message type and add it to both allocator maps
	 */
	template <class Msg>
	inline void add_allocator_to_maps()
	{
		message_allocator_impl<Msg> *alloc = new message_allocator_impl<Msg>();
		byInt[alloc->getMessageID()] = alloc;
		byStr[alloc->getMessageType()] = alloc;
	}

	/**
	 * The variadic version of the initialize function that adds one message allocator, then
	 * recurses across the others.
	 */
	template <class... Others, class Msg>
	inline void initialize_maps()
	{
		add_allocator_to_maps<Msg>();
		initialize_maps<Others...>();	// Recursive call
	}

	/// An object to hold any error messages
	std::unique_ptr<J2735Exception> error_message;
public:
	/**
	 * Creates a new factory to use.  The allocator maps will only be initialized once.
	 */
	J2735MessageFactory():
		byInt(get_int_map()),
		byStr(get_str_map())
	{
		if (byInt.size() <= 0 || byStr.size() <= 0)
		{
			add_allocator_to_maps<BsmMessage>();
			add_allocator_to_maps<BsmvMessage>();
			add_allocator_to_maps<CsrMessage>();
			add_allocator_to_maps<EvaMessage>();
			add_allocator_to_maps<IntersectionCollisionMessage>();
			add_allocator_to_maps<MapDataMessage>();
			add_allocator_to_maps<NmeaMessage>();
			add_allocator_to_maps<PdmMessage>();
			add_allocator_to_maps<PmmMessage>();
			add_allocator_to_maps<PvdMessage>();
			add_allocator_to_maps<PsmMessage>();
			add_allocator_to_maps<RsaMessage>();
			add_allocator_to_maps<RtcmMessage>();
			add_allocator_to_maps<SrmMessage>();
			add_allocator_to_maps<SsmMessage>();
			add_allocator_to_maps<SpatMessage>();
			add_allocator_to_maps<TimMessage>();
			add_allocator_to_maps<UperFrameMessage>();
		}
	}

	/**
	 * Return the last event that occurred.  This can be used to report on the
	 * error that happened while creating the message
	 * @return The last event, or an empty event if none occurred
	 */
	inline J2735Exception get_event()
	{
		if (error_message)
			return *error_message;
		else
			return J2735Exception("No event");
	}

	/**
	 * Initialize the J2735 routeable message with the given byte stream.
	 *
	 * @param The J2735 message to initialize
	 * @param The byte stream
	 */
	void initialize(TmxJ2735EncodedMessageBase *msg, const tmx::byte_stream &bytes)
	{
		if (msg)
		{
			// The encoding will be overwritten as a string, so save it
			std::string enc = msg->get_encoding();
			msg->set_payload_bytes(bytes);
			msg->set_encoding(enc);
		}
	}

	/**
	 * Create a new J2735 Message from the given message ID.
	 *
	 * @param messageId The J2735 message ID
	 * @return A new message of that type, or NULL if an error occurs
	 */
	inline TmxJ2735EncodedMessageBase *NewMessage(int messageId)
	{
		error_message.reset();

		try
		{
			return byInt.at(messageId)->allocate();
		}
		catch (std::exception &ex)
		{
			error_message.reset(new J2735Exception(ex));
			*error_message << errmsg_info{"Unable to create new J2735 message from message ID " + std::to_string(messageId)};
			return NULL;
		}
	}

	/**
	 * Create a new J2735 Message from the given message type.
	 *
	 * @param messageType The type name for the J2735 Message
	 * @return A new message of that type, or NULL if an error occurs
	 */
	inline TmxJ2735EncodedMessageBase *NewMessage(std::string messageType)
	{
		error_message.reset();

		try
		{
			return byStr.at(messageType)->allocate();
		}
		catch (std::exception &ex)
		{
			error_message.reset(new J2735Exception(ex));
			*error_message << errmsg_info{"Unable to create new J2735 message from message type " + messageType};
			return NULL;
		}
	}

	/**
	 * Create a new J2735 Routeable Message from the given byte stream.  The message ID is separated
	 * from the byte stream to determine what message type to create.
	 *
	 * @param bytes The DER encoded set of bytes
	 * @return A new message of that type, or NULL if an error occurs
	 */
	inline TmxJ2735EncodedMessageBase *NewMessage(tmx::byte_stream &bytes)
	{
		error_message.reset();

		tmx::messages::codec::der<UperFrameMessage> derDecoder;

		int id = derDecoder.decode_contentId(bytes);
		if (id > 0)
		{
			TmxJ2735EncodedMessageBase *msg = NewMessage(id);
			initialize(msg, bytes);
			return msg;
		}

		error_message.reset(new J2735Exception("Failed to create new J2735 message"));
		*error_message << errmsg_info{"Unable to determine message ID from bytes: " +
			battelle::attributes::attribute_lexical_cast<std::string>(bytes)};
		return NULL;
	}

};

} /* End namespace messages */
} /* End namespace tmx */

#endif /* TMX_J2735_MESSAGES_J2735MESSAGEFACTORY_HPP_ */
