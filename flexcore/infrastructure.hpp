#ifndef SRC_INFRASTRUCTURE_HPP_
#define SRC_INFRASTRUCTURE_HPP_

#include <flexcore/extended/base_node.hpp>
#include <flexcore/scheduler/cyclecontrol.hpp>

namespace fc
{
namespace detail {
class region_factory;
}

class infrastructure
{
public:
	infrastructure();
	~infrastructure();

	std::shared_ptr<parallel_region> add_region(const std::string& name,
			const virtual_clock::steady::duration& tick_rate);

	owning_base_node& node_owner() { return forest_root.nodes(); }
	void infinite_main_loop();
	void start_scheduler() { scheduler.start(); }
	void stop_scheduler() { scheduler.stop(); }
	void iterate_main_loop();

private:
//	graph::connection_graph abstract_graph;
	thread::cycle_control scheduler;
	std::shared_ptr<detail::region_factory> region_maker;
	root_node forest_root;
};

} /* namespace fc */

#endif /* SRC_INFRASTRUCTURE_HPP_ */
