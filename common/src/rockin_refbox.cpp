
#include <mir_rockin_refbox/rockin_refbox.h>


#include <raw_refbox_comm/BeaconSignal.pb.h>
#include <raw_refbox_comm/VersionInfo.pb.h>
#include <raw_refbox_comm/BenchmarkState.pb.h>
#include <raw_refbox_comm/Inventory.pb.h>
#include <raw_refbox_comm/Order.pb.h>
#include <raw_refbox_comm/DrillingMachine.pb.h>
#include <raw_refbox_comm/ConveyorBelt.pb.h>
#include <raw_refbox_comm/Camera.pb.h>
#include <raw_refbox_comm/Image.pb.h>
#include <raw_refbox_comm/BenchmarkFeedback.pb.h>
#include <raw_refbox_comm/Time.pb.h>

using namespace rockin_msgs;

RockinRefbox::RockinRefbox(const std::string &name, const std::string &team_name, const std::string &host, int recv_port,
        int send_port) : name_(name), team_name_(team_name), host_(host), recv_port_(recv_port), send_port_(send_port), sequence_number_(0)
{
    std::cout << "starting rockin ref box" << std::endl;
    peer_public_ = new ProtobufBroadcastPeer(host_,recv_port_,send_port_);
    std::cout << "created broadcast peer" << std::endl;
    MessageRegister &message_register = peer_public_->message_register();
    std::cout << "got message register" << std::endl;
    message_register.add_message_type<BeaconSignal>();
    message_register.add_message_type<BenchmarkState>();
    /*
    message_register.add_message_type<VersionInfo>();
    message_register.add_message_type<Inventory>();
    message_register.add_message_type<OrderInfo>();
    message_register.add_message_type<DrillingMachineStatus>();
    message_register.add_message_type<ConveyorBeltStatus>();
    message_register.add_message_type<Image>();
    message_register.add_message_type<BenchmarkFeedback>();
,
    */
    std::cout << "registered messages " << std::endl;
    peer_public_->signal_received().connect(boost::bind(&RockinRefbox::handle_message, this, _1, _2, _3, _4));
    peer_public_->signal_recv_error().connect(boost::bind(&RockinRefbox::handle_recv_error,this, _1, _2));
    peer_public_->signal_send_error().connect(boost::bind(&RockinRefbox::handle_send_error, this, _1));

#define BOOST_T_TIMER
#ifdef BOOST_T_TIMER
    boost::asio::io_service io_service;
    timer_ = new boost::asio::deadline_timer(io_service);
    timer_->expires_from_now(boost::posix_time::milliseconds(2000));
    timer_->async_wait(boost::bind(&RockinRefbox::send_beacon_signal, this));
//    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
//    signals.async_wait(boost::bind(&RockinRefbox::service_signals, this, _1, _2));
    io_service.run();
#else
    while(true)
    {
        send_beacon_signal();
    }
#endif
}


RockinRefbox::~RockinRefbox()
{
    delete timer_;
    delete peer_public_;
}

void RockinRefbox::send_beacon_signal()
{
    std::cout << "sending beacon" << std::endl;
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
    std::shared_ptr<BeaconSignal> signal(new BeaconSignal());
    Time *time = signal->mutable_time();
    boost::posix_time::time_duration const since_epoch =
      now - boost::posix_time::from_time_t(0);

    time->set_sec(static_cast<google::protobuf::int64>(since_epoch.total_seconds()));
    time->set_nsec(
      static_cast<google::protobuf::int64>(since_epoch.fractional_seconds() * 
             (1000000000/since_epoch.ticks_per_second())));

    signal->set_peer_name(name_);
    signal->set_team_name(team_name_);
    signal->set_seq(++sequence_number_);
    peer_public_->send(signal);

        /*

        // Request a camera image
        CameraCommand cam_cmd;
        peer_team_->send(cam_cmd);


        // Send benchnmark feedback
        BenchmarkFeedback bf;
        bf.mutable_object_pose()->mutable_position()->set_x(0.0);
        bf.mutable_object_pose()->mutable_position()->set_y(0.0);
        bf.mutable_object_pose()->mutable_position()->set_z(0.0);
        bf.mutable_object_pose()->mutable_orientation()->set_w(0.0);
        bf.mutable_object_pose()->mutable_orientation()->set_x(1.0);
        bf.mutable_object_pose()->mutable_orientation()->set_y(0.0);
        bf.mutable_object_pose()->mutable_orientation()->set_z(0.0);
        bf.mutable_end_effector_pose()->mutable_position()->set_x(0.0);
        bf.mutable_end_effector_pose()->mutable_position()->set_y(0.0);
        bf.mutable_end_effector_pose()->mutable_position()->set_z(0.0);
        bf.mutable_end_effector_pose()->mutable_orientation()->set_w(0.0);
        bf.mutable_end_effector_pose()->mutable_orientation()->set_x(1.0);
        bf.mutable_end_effector_pose()->mutable_orientation()->set_y(0.0);
        bf.mutable_end_effector_pose()->mutable_orientation()->set_z(0.0);
        bf.set_object_instance_name("AX-01");
        bf.set_object_class_name("aluminium");
        bf.set_grasp_notification(true);
        peer_public_->send(bf); */
#ifdef BOOST_T_TIMER
    timer_->expires_at(timer_->expires_at()
          + boost::posix_time::milliseconds(2000));
    timer_->async_wait(boost::bind(&RockinRefbox::send_beacon_signal,this));
#endif
}

void RockinRefbox::service_signals(const boost::system::error_code &error, int signum)
{
    if (!error)
    {
        if (timer_)
        {
            timer_->cancel();
        }
    }
}

void RockinRefbox::handle_recv_error(boost::asio::ip::udp::endpoint &endpoint, std::string msg)
{

}


void RockinRefbox::handle_send_error(std::string msg)
{
}


void RockinRefbox::handle_message(boost::asio::ip::udp::endpoint &sender, uint16_t component_id, uint16_t msg_type, std::shared_ptr<google::protobuf::Message> msg)
{
    std::cout << "received message " << std::endl;
    std::shared_ptr<BenchmarkState> bs;
    if ((bs = std::dynamic_pointer_cast<BenchmarkState>(msg)))
    {
        std::cout << "BenchmarkState received:" << std::endl;

        std::cout << "  Time: " << bs->benchmark_time().sec() << "s" << std::endl;

        std::cout << "  Phase: ";
        switch (bs->phase().type())
        {
            case BenchmarkPhase::NONE: std::cout << "NONE"; break;
            case BenchmarkPhase::FBM: std::cout << "FBM"; break;
            case BenchmarkPhase::TBM: std::cout << "TBM"; break;
        }
        std::cout << " " << bs->phase().type_id();
        if (bs->phase().has_description()) std::cout << " (" << bs->phase().description() << ")";
        std::cout << std::endl;

        std::cout << "  State: ";
        switch (bs->state())
        {
            case BenchmarkState::INIT: std::cout << "INIT" << std::endl; break;
            case BenchmarkState::RUNNING: std::cout << "RUNNING" << std::endl; break;
            case BenchmarkState::PAUSED: std::cout << "PAUSED" << std::endl; break;
            case BenchmarkState::FINISHED: std::cout << "FINISHED" << std::endl; break;
        }
    }
}
