#include "stdafx.h"

#include "thread.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/ObjectFactory.h"

namespace
{
    using Factory = Utils::ObjectFactory<Data::ParentTopic, Data::ParentTopic::Type>;
    std::unique_ptr<Factory> parentTopicFactory = []()
    {
        auto factory = std::make_unique<Factory>();
        factory->registerClass<Data::MessageParentTopic>(Data::ParentTopic::Type::message);
        factory->registerClass<Data::TaskParentTopic>(Data::ParentTopic::Type::task);
        return factory;
    }();
}

namespace Data
{
    ParentTopic::Type ParentTopic::unserialize_type(const core::coll_helper& _coll)
    {
        Ui::gui_coll_helper parentColl(_coll.get_value_as_collection("parentTopic"), false);

        if (parentColl.is_value_exist("type"))
            return parentColl.get_value_as_enum<Type>("type");
        im_assert(!"parent topic type dont exists");
        return Type::invalid;
    }

    std::shared_ptr<ParentTopic> ParentTopic::createParentTopic(Type _type)
    {
        return parentTopicFactory->createShared(_type);;
    }

    QString MessageParentTopic::getChat(const std::shared_ptr<ParentTopic>& _topic)
    {
        if (!_topic)
            return {};
        if (const auto type = _topic->type_; Data::ParentTopic::Type::message == type)
        {
            const auto messageTopic = std::static_pointer_cast<Data::MessageParentTopic>(_topic);
            return messageTopic->chat_;
        }
        return {};
    }

    void MessageParentTopic::serialize(core::coll_helper& _coll) const
    {
        Ui::gui_coll_helper parentColl(_coll->create_collection(), false);
        parentColl.set_value_as_qstring("chat", chat_);
        parentColl.set_value_as_int64("msgId", msgId_);
        parentColl.set_value_as_enum("type", Type::message);

        _coll.set_value_as_collection("parentTopic", parentColl.get());
    }

    void MessageParentTopic::unserialize(const core::coll_helper& _coll)
    {
        im_assert(unserialize_type(_coll) == Type::message);
        Ui::gui_coll_helper parentColl(_coll.get_value_as_collection("parentTopic"), false);
        chat_ = parentColl.get<QString>("chat");
        msgId_ = parentColl.get_value_as_int64("msgId");
    }

    void ThreadUpdate::unserialize(const core::coll_helper& _coll)
    {
        threadId_ = _coll.get<QString>("id");
        repliesCount_ = _coll.get_value_as_int("repliesCount");
        unreadMessages_ = _coll.get_value_as_int("unreadMsg");
        unreadMentionsMe_ = _coll.get_value_as_int("unreadMention");
        yoursLastRead_ = _coll.get_value_as_int64("lastRead");
        yoursLastReadMention_ = _coll.get_value_as_int64("lastReadMention");
        isSubscriber_ = _coll.get_value_as_bool("subscriber");

        const auto type = ParentTopic::unserialize_type(_coll);
        if (Data::ParentTopic::Type::invalid != type)
        {
            parent_ = parentTopicFactory->createShared(type);
            parent_->unserialize(_coll);
        }
    }

    void TaskParentTopic::serialize(core::coll_helper& _coll) const
    {
        Ui::gui_coll_helper parentColl(_coll->create_collection(), false);
        parentColl.set_value_as_qstring("taskId", taskId_);
        parentColl.set_value_as_enum("type", Type::task);

        _coll.set_value_as_collection("parentTopic", parentColl.get());
    }

    void TaskParentTopic::unserialize(const core::coll_helper& _coll)
    {
        im_assert(unserialize_type(_coll) == Type::task);
        Ui::gui_coll_helper parentColl(_coll.get_value_as_collection("parentTopic"), false);
        taskId_ = parentColl.get<QString>("taskId");
    }
}