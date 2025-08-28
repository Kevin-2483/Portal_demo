#include "portal_console.h"
#include <algorithm>
#include <iomanip>

using namespace Portal::Example;

namespace Portal {
namespace Console {

    PortalConsole::PortalConsole() 
        : next_entity_id_(1000)
        , running_(false) 
    {
        // 创建所有接口实现
        physics_query_ = std::make_unique<ExamplePhysicsQuery>();
        physics_manipulator_ = std::make_unique<ExamplePhysicsManipulator>(physics_query_.get());
        render_query_ = std::make_unique<ExampleRenderQuery>();
        render_manipulator_ = std::make_unique<ExampleRenderManipulator>();
        event_handler_ = std::make_unique<ExampleEventHandler>();
        
        // 设置接口容器
        interfaces_.physics_query = physics_query_.get();
        interfaces_.physics_manipulator = physics_manipulator_.get();
        interfaces_.render_query = render_query_.get();
        interfaces_.render_manipulator = render_manipulator_.get();
        interfaces_.event_handler = event_handler_.get();
        
        // 创建传送门管理器
        portal_manager_ = std::make_unique<PortalManager>(interfaces_);
        
        // 设置命令
        setup_commands();
    }

    bool PortalConsole::initialize() {
        if (!portal_manager_->initialize()) {
            std::cout << "Failed to initialize portal system!\n";
            return false;
        }
        
        std::cout << "Portal Console System initialized successfully.\n";
        return true;
    }

    void PortalConsole::run() {
        print_banner();
        
        if (!initialize()) {
            return;
        }
        
        running_ = true;
        std::string input;
        
        while (running_) {
            std::cout << "\nPortal> ";
            std::getline(std::cin, input);
            
            if (!input.empty()) {
                execute_command(input);
            }
        }
        
        shutdown();
    }

    void PortalConsole::shutdown() {
        portal_manager_->shutdown();
        std::cout << "Portal Console System shutdown.\n";
    }

    void PortalConsole::execute_command(const std::string& command) {
        auto args = split_command(command);
        if (args.empty()) return;
        
        std::string cmd = args[0];
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
        
        auto it = commands_.find(cmd);
        if (it != commands_.end()) {
            try {
                it->second(args);
            } catch (const std::exception& e) {
                std::cout << "Error executing command: " << e.what() << "\n";
            }
        } else {
            std::cout << "Unknown command: " << cmd << ". Type 'help' for available commands.\n";
        }
    }

    void PortalConsole::setup_commands() {
        commands_["help"] = [this](const auto& args) { cmd_help(args); };
        commands_["h"] = [this](const auto& args) { cmd_help(args); };
        commands_["status"] = [this](const auto& args) { cmd_status(args); };
        commands_["create_portal"] = [this](const auto& args) { cmd_create_portal(args); };
        commands_["cp"] = [this](const auto& args) { cmd_create_portal(args); };
        commands_["link_portals"] = [this](const auto& args) { cmd_link_portals(args); };
        commands_["link"] = [this](const auto& args) { cmd_link_portals(args); };
        commands_["list_portals"] = [this](const auto& args) { cmd_list_portals(args); };
        commands_["lp"] = [this](const auto& args) { cmd_list_portals(args); };
        commands_["create_entity"] = [this](const auto& args) { cmd_create_entity(args); };
        commands_["ce"] = [this](const auto& args) { cmd_create_entity(args); };
        commands_["list_entities"] = [this](const auto& args) { cmd_list_entities(args); };
        commands_["le"] = [this](const auto& args) { cmd_list_entities(args); };
        commands_["move_entity"] = [this](const auto& args) { cmd_move_entity(args); };
        commands_["move"] = [this](const auto& args) { cmd_move_entity(args); };
        commands_["teleport"] = [this](const auto& args) { cmd_teleport_entity(args); };
        commands_["tp"] = [this](const auto& args) { cmd_teleport_entity(args); };
        commands_["update"] = [this](const auto& args) { cmd_update(args); };
        commands_["u"] = [this](const auto& args) { cmd_update(args); };
        commands_["set_velocity"] = [this](const auto& args) { cmd_set_entity_velocity(args); };
        commands_["vel"] = [this](const auto& args) { cmd_set_entity_velocity(args); };
        commands_["set_portal_velocity"] = [this](const auto& args) { cmd_set_portal_velocity(args); };
        commands_["pvel"] = [this](const auto& args) { cmd_set_portal_velocity(args); };
        commands_["teleport_with_velocity"] = [this](const auto& args) { cmd_teleport_with_velocity(args); };
        commands_["tpv"] = [this](const auto& args) { cmd_teleport_with_velocity(args); };
        commands_["test_moving_portal"] = [this](const auto& args) { cmd_test_moving_portal(args); };
        commands_["tmp"] = [this](const auto& args) { cmd_test_moving_portal(args); };
        commands_["debug_collision"] = [this](const auto& args) { cmd_debug_collision(args); };
        commands_["dbg"] = [this](const auto& args) { cmd_debug_collision(args); };
        commands_["simulate_collision"] = [this](const auto& args) { cmd_simulate_collision_detection(args); };
        commands_["scol"] = [this](const auto& args) { cmd_simulate_collision_detection(args); };
        commands_["simulate"] = [this](const auto& args) { cmd_simulate(args); };
        commands_["sim"] = [this](const auto& args) { cmd_simulate(args); };
        commands_["info"] = [this](const auto& args) { cmd_get_entity_info(args); };
        commands_["destroy_portal"] = [this](const auto& args) { cmd_destroy_portal(args); };
        commands_["dp"] = [this](const auto& args) { cmd_destroy_portal(args); };
        commands_["exit"] = [this](const auto& args) { cmd_exit(args); };
        commands_["quit"] = [this](const auto& args) { cmd_exit(args); };
    }

    void PortalConsole::cmd_help(const std::vector<std::string>& args) {
        std::cout << "\n=== Portal Console Commands ===\n";
        std::cout << "\nPortal Management:\n";
        std::cout << "  create_portal <name> <x> <y> <z> <nx> <ny> <nz> [width] [height]\n";
        std::cout << "    cp <name> <x> <y> <z> <nx> <ny> <nz> [width] [height] - Create portal\n";
        std::cout << "  link_portals <portal1> <portal2>\n";
        std::cout << "    link <portal1> <portal2> - Link two portals\n";
        std::cout << "  list_portals\n";
        std::cout << "    lp - List all portals\n";
        std::cout << "  destroy_portal <name>\n";
        std::cout << "    dp <name> - Destroy portal\n";
        
        std::cout << "\nEntity Management:\n";
        std::cout << "  create_entity <name> <x> <y> <z>\n";
        std::cout << "    ce <name> <x> <y> <z> - Create entity\n";
        std::cout << "  list_entities\n";
        std::cout << "    le - List all entities\n";
        std::cout << "  move_entity <name> <x> <y> <z>\n";
        std::cout << "    move <name> <x> <y> <z> - Move entity\n";
        std::cout << "  set_velocity <name> <vx> <vy> <vz>\n";
        std::cout << "    vel <name> <vx> <vy> <vz> - Set entity velocity\n";
        std::cout << "  info <name> - Get entity information\n";
        
        std::cout << "\nTeleportation:\n";
        std::cout << "  teleport <entity> <source_portal> <target_portal>\n";
        std::cout << "    tp <entity> <source_portal> <target_portal> - Manual teleport\n";
        std::cout << "  teleport_with_velocity <entity> <source_portal> <target_portal>\n";
        std::cout << "    tpv <entity> <source_portal> <target_portal> - Teleport considering portal velocities\n";
        
        std::cout << "\nVelocity & Physics:\n";
        std::cout << "  set_velocity <name> <vx> <vy> <vz>\n";
        std::cout << "    vel <name> <vx> <vy> <vz> - Set entity velocity\n";
        std::cout << "  set_portal_velocity <portal> <vx> <vy> <vz> [avx] [avy] [avz]\n";
        std::cout << "    pvel <portal> <vx> <vy> <vz> [avx] [avy] [avz] - Set portal velocity\n";
        std::cout << "  test_moving_portal <portal> <vx> <vy> <vz> <duration>\n";
        std::cout << "    tmp <portal> <vx> <vy> <vz> <duration> - Test moving portal scenario\n";
        std::cout << "  debug_collision <entity> <portal>\n";
        std::cout << "    dbg <entity> <portal> - Debug collision detection details\n";
        std::cout << "  simulate_collision <duration> [fps]\n";
        std::cout << "    scol <duration> [fps] - Simulate engine collision detection\n";
        std::cout << "  simulate <duration> [fps]\n";
        std::cout << "    sim <duration> [fps] - Simulate physics for given time\n";
        
        std::cout << "\nSystem:\n";
        std::cout << "  status - Show system status\n";
        std::cout << "  update [count] - Update system (default: 1 frame)\n";
        std::cout << "    u [count] - Update system\n";
        std::cout << "  help - Show this help\n";
        std::cout << "    h - Show help\n";
        std::cout << "  exit - Exit console\n";
        std::cout << "    quit - Exit console\n";
        std::cout << "\nExample:\n";
        std::cout << "  cp portal1 -5 0 0 1 0 0\n";
        std::cout << "  cp portal2 5 0 0 -1 0 0\n";
        std::cout << "  link portal1 portal2\n";
        std::cout << "  ce player 0 0 0\n";
        std::cout << "  tp player portal1 portal2\n";
    }

    void PortalConsole::cmd_status(const std::vector<std::string>& args) {
        std::cout << "\n=== Portal System Status ===\n";
        std::cout << "Portal count: " << portal_manager_->get_portal_count() << "\n";
        std::cout << "Registered entities: " << portal_manager_->get_registered_entity_count() << "\n";
        std::cout << "Teleporting entities: " << portal_manager_->get_teleporting_entity_count() << "\n";
        std::cout << "System version: " << get_version_string() << "\n";
    }

    void PortalConsole::cmd_create_portal(const std::vector<std::string>& args) {
        if (args.size() < 8) {
            std::cout << "Usage: create_portal <name> <x> <y> <z> <nx> <ny> <nz> [width] [height]\n";
            return;
        }
        
        std::string name = args[1];
        Vector3 center(std::stof(args[2]), std::stof(args[3]), std::stof(args[4]));
        Vector3 normal(std::stof(args[5]), std::stof(args[6]), std::stof(args[7]));
        normal = normal.normalized();
        
        PortalPlane plane;
        plane.center = center;
        plane.normal = normal;
        plane.width = (args.size() > 8) ? std::stof(args[8]) : 2.0f;
        plane.height = (args.size() > 9) ? std::stof(args[9]) : 3.0f;
        
        // 计算 up 和 right 向量
        Vector3 world_up(0, 1, 0);
        if (std::abs(normal.dot(world_up)) > 0.99f) {
            world_up = Vector3(0, 0, 1);
        }
        plane.right = normal.cross(world_up).normalized();
        plane.up = plane.right.cross(normal).normalized();
        
        PortalId portal_id = portal_manager_->create_portal(plane);
        portal_names_[portal_id] = name;
        
        std::cout << "Created portal '" << name << "' (ID: " << portal_id << ")\n";
        std::cout << "  Position: (" << center.x << ", " << center.y << ", " << center.z << ")\n";
        std::cout << "  Normal: (" << normal.x << ", " << normal.y << ", " << normal.z << ")\n";
        std::cout << "  Size: " << plane.width << " x " << plane.height << "\n";
    }

    void PortalConsole::cmd_link_portals(const std::vector<std::string>& args) {
        if (args.size() < 3) {
            std::cout << "Usage: link_portals <portal1> <portal2>\n";
            return;
        }
        
        PortalId portal1 = find_portal_by_name(args[1]);
        PortalId portal2 = find_portal_by_name(args[2]);
        
        if (portal1 == INVALID_PORTAL_ID) {
            std::cout << "Portal '" << args[1] << "' not found.\n";
            return;
        }
        
        if (portal2 == INVALID_PORTAL_ID) {
            std::cout << "Portal '" << args[2] << "' not found.\n";
            return;
        }
        
        if (portal_manager_->link_portals(portal1, portal2)) {
            std::cout << "Successfully linked portals '" << args[1] << "' and '" << args[2] << "'.\n";
        } else {
            std::cout << "Failed to link portals.\n";
        }
    }

    void PortalConsole::cmd_list_portals(const std::vector<std::string>& args) {
        std::cout << "\n=== Portal List ===\n";
        if (portal_names_.empty()) {
            std::cout << "No portals created.\n";
            return;
        }
        
        for (const auto& [portal_id, name] : portal_names_) {
            const Portal* portal = portal_manager_->get_portal(portal_id);
            if (portal) {
                print_portal_info(portal, name);
            }
        }
    }

    void PortalConsole::cmd_create_entity(const std::vector<std::string>& args) {
        if (args.size() < 5) {
            std::cout << "Usage: create_entity <name> <x> <y> <z>\n";
            return;
        }
        
        std::string name = args[1];
        EntityId entity_id = next_entity_id_++;
        
        Transform transform;
        transform.position = Vector3(std::stof(args[2]), std::stof(args[3]), std::stof(args[4]));
        
        Vector3 bounds_min(-0.5f, -0.5f, -0.5f);
        Vector3 bounds_max(0.5f, 0.5f, 0.5f);
        
        get_physics_query()->add_test_entity(entity_id, transform, bounds_min, bounds_max);
        portal_manager_->register_entity(entity_id);
        entity_names_[entity_id] = name;
        
        std::cout << "Created entity '" << name << "' (ID: " << entity_id << ")\n";
        std::cout << "  Position: (" << transform.position.x << ", " << transform.position.y 
                  << ", " << transform.position.z << ")\n";
    }

    void PortalConsole::cmd_list_entities(const std::vector<std::string>& args) {
        std::cout << "\n=== Entity List ===\n";
        if (entity_names_.empty()) {
            std::cout << "No entities created.\n";
            return;
        }
        
        for (const auto& [entity_id, name] : entity_names_) {
            print_entity_info(entity_id, name);
        }
    }

    void PortalConsole::cmd_move_entity(const std::vector<std::string>& args) {
        if (args.size() < 5) {
            std::cout << "Usage: move_entity <name> <x> <y> <z>\n";
            return;
        }
        
        EntityId entity_id = find_entity_by_name(args[1]);
        if (entity_id == INVALID_ENTITY_ID) {
            std::cout << "Entity '" << args[1] << "' not found.\n";
            return;
        }
        
        Transform current_transform = interfaces_.physics_query->get_entity_transform(entity_id);
        current_transform.position = Vector3(std::stof(args[2]), std::stof(args[3]), std::stof(args[4]));
        
        interfaces_.physics_manipulator->set_entity_transform(entity_id, current_transform);
        get_physics_query()->update_entity_transform(entity_id, current_transform);
        
        std::cout << "Moved entity '" << args[1] << "' to (" << current_transform.position.x 
                  << ", " << current_transform.position.y << ", " << current_transform.position.z << ")\n";
    }

    void PortalConsole::cmd_teleport_entity(const std::vector<std::string>& args) {
        if (args.size() < 4) {
            std::cout << "Usage: teleport <entity> <source_portal> <target_portal>\n";
            return;
        }
        
        EntityId entity_id = find_entity_by_name(args[1]);
        PortalId source_portal = find_portal_by_name(args[2]);
        PortalId target_portal = find_portal_by_name(args[3]);
        
        if (entity_id == INVALID_ENTITY_ID) {
            std::cout << "Entity '" << args[1] << "' not found.\n";
            return;
        }
        
        if (source_portal == INVALID_PORTAL_ID) {
            std::cout << "Source portal '" << args[2] << "' not found.\n";
            return;
        }
        
        if (target_portal == INVALID_PORTAL_ID) {
            std::cout << "Target portal '" << args[3] << "' not found.\n";
            return;
        }
        
        TeleportResult result = portal_manager_->teleport_entity(entity_id, source_portal, target_portal);
        
        switch (result) {
            case TeleportResult::SUCCESS:
                std::cout << "Successfully teleported entity '" << args[1] << "'.\n";
                break;
            case TeleportResult::FAILED_INVALID_PORTAL:
                std::cout << "Teleport failed: Invalid portal.\n";
                break;
            case TeleportResult::FAILED_BLOCKED:
                std::cout << "Teleport failed: Target position blocked.\n";
                break;
            case TeleportResult::FAILED_TOO_LARGE:
                std::cout << "Teleport failed: Entity too large.\n";
                break;
            default:
                std::cout << "Teleport failed: Unknown error.\n";
                break;
        }
    }

    void PortalConsole::cmd_update(const std::vector<std::string>& args) {
        int frame_count = 1;
        if (args.size() > 1) {
            frame_count = std::stoi(args[1]);
        }
        
        std::cout << "Updating system for " << frame_count << " frame(s)...\n";
        
        for (int i = 0; i < frame_count; ++i) {
            portal_manager_->update(0.016f); // 模拟60FPS
        }
        
        std::cout << "Update complete.\n";
    }

    void PortalConsole::cmd_set_entity_velocity(const std::vector<std::string>& args) {
        if (args.size() < 5) {
            std::cout << "Usage: set_velocity <name> <vx> <vy> <vz>\n";
            return;
        }
        
        EntityId entity_id = find_entity_by_name(args[1]);
        if (entity_id == INVALID_ENTITY_ID) {
            std::cout << "Entity '" << args[1] << "' not found.\n";
            return;
        }
        
        PhysicsState physics_state;
        physics_state.linear_velocity = Vector3(std::stof(args[2]), std::stof(args[3]), std::stof(args[4]));
        
        interfaces_.physics_manipulator->set_entity_physics_state(entity_id, physics_state);
        get_physics_query()->update_entity_physics_state(entity_id, physics_state);
        
        std::cout << "Set velocity of entity '" << args[1] << "' to (" 
                  << physics_state.linear_velocity.x << ", " << physics_state.linear_velocity.y 
                  << ", " << physics_state.linear_velocity.z << ")\n";
    }

    void PortalConsole::cmd_get_entity_info(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            std::cout << "Usage: info <name>\n";
            return;
        }
        
        EntityId entity_id = find_entity_by_name(args[1]);
        if (entity_id == INVALID_ENTITY_ID) {
            std::cout << "Entity '" << args[1] << "' not found.\n";
            return;
        }
        
        std::cout << "\n=== Entity Information ===\n";
        print_entity_info(entity_id, args[1]);
        
        // 检查传送状态
        const TeleportState* teleport_state = portal_manager_->get_entity_teleport_state(entity_id);
        if (teleport_state && teleport_state->is_teleporting) {
            std::cout << "  Teleporting: YES (Progress: " << (teleport_state->transition_progress * 100) << "%)\n";
            std::cout << "  Source Portal: " << teleport_state->source_portal << "\n";
            std::cout << "  Target Portal: " << teleport_state->target_portal << "\n";
        } else {
            std::cout << "  Teleporting: NO\n";
        }
    }

    void PortalConsole::cmd_destroy_portal(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            std::cout << "Usage: destroy_portal <name>\n";
            return;
        }
        
        PortalId portal_id = find_portal_by_name(args[1]);
        if (portal_id == INVALID_PORTAL_ID) {
            std::cout << "Portal '" << args[1] << "' not found.\n";
            return;
        }
        
        portal_manager_->destroy_portal(portal_id);
        portal_names_.erase(portal_id);
        
        std::cout << "Destroyed portal '" << args[1] << "'.\n";
    }

    void PortalConsole::cmd_set_portal_velocity(const std::vector<std::string>& args) {
        if (args.size() < 5) {
            std::cout << "Usage: set_portal_velocity <portal> <vx> <vy> <vz> [avx] [avy] [avz]\n";
            std::cout << "  Set linear and optional angular velocity for a portal\n";
            return;
        }
        
        PortalId portal_id = find_portal_by_name(args[1]);
        if (portal_id == INVALID_PORTAL_ID) {
            std::cout << "Portal '" << args[1] << "' not found.\n";
            return;
        }
        
        PhysicsState portal_physics;
        portal_physics.linear_velocity = Vector3(std::stof(args[2]), std::stof(args[3]), std::stof(args[4]));
        
        if (args.size() >= 8) {
            portal_physics.angular_velocity = Vector3(std::stof(args[5]), std::stof(args[6]), std::stof(args[7]));
        }
        
        portal_manager_->update_portal_physics_state(portal_id, portal_physics);
        
        std::cout << "Set portal '" << args[1] << "' velocity:\n";
        std::cout << "  Linear: (" << portal_physics.linear_velocity.x << ", " 
                  << portal_physics.linear_velocity.y << ", " << portal_physics.linear_velocity.z << ")\n";
        if (args.size() >= 8) {
            std::cout << "  Angular: (" << portal_physics.angular_velocity.x << ", " 
                      << portal_physics.angular_velocity.y << ", " << portal_physics.angular_velocity.z << ")\n";
        }
    }

    void PortalConsole::cmd_teleport_with_velocity(const std::vector<std::string>& args) {
        if (args.size() < 4) {
            std::cout << "Usage: teleport_with_velocity <entity> <source_portal> <target_portal>\n";
            std::cout << "  Teleport entity considering portal velocities\n";
            return;
        }
        
        EntityId entity_id = find_entity_by_name(args[1]);
        PortalId source_portal = find_portal_by_name(args[2]);
        PortalId target_portal = find_portal_by_name(args[3]);
        
        if (entity_id == INVALID_ENTITY_ID) {
            std::cout << "Entity '" << args[1] << "' not found.\n";
            return;
        }
        
        if (source_portal == INVALID_PORTAL_ID) {
            std::cout << "Source portal '" << args[2] << "' not found.\n";
            return;
        }
        
        if (target_portal == INVALID_PORTAL_ID) {
            std::cout << "Target portal '" << args[3] << "' not found.\n";
            return;
        }
        
        TeleportResult result = portal_manager_->teleport_entity_with_velocity(entity_id, source_portal, target_portal);
        
        switch (result) {
            case TeleportResult::SUCCESS: {
                std::cout << "Successfully teleported entity '" << args[1] << "' with velocity consideration.\n";
                
                // 显示传送后的状态
                PhysicsState new_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);
                std::cout << "  New velocity: (" << new_physics.linear_velocity.x << ", " 
                          << new_physics.linear_velocity.y << ", " << new_physics.linear_velocity.z << ")\n";
                break;
            }
            case TeleportResult::FAILED_INVALID_PORTAL:
                std::cout << "Teleport failed: Invalid portal.\n";
                break;
            case TeleportResult::FAILED_BLOCKED:
                std::cout << "Teleport failed: Target position blocked.\n";
                break;
            case TeleportResult::FAILED_TOO_LARGE:
                std::cout << "Teleport failed: Entity too large.\n";
                break;
            default:
                std::cout << "Teleport failed: Unknown error.\n";
                break;
        }
    }

    void PortalConsole::cmd_test_moving_portal(const std::vector<std::string>& args) {
        if (args.size() < 6) {
            std::cout << "Usage: test_moving_portal <portal> <vx> <vy> <vz> <duration>\n";
            std::cout << "  Test scenario: Move a portal and check collisions with entities\n";
            return;
        }
        
        PortalId portal_id = find_portal_by_name(args[1]);
        if (portal_id == INVALID_PORTAL_ID) {
            std::cout << "Portal '" << args[1] << "' not found.\n";
            return;
        }
        
        Vector3 velocity(std::stof(args[2]), std::stof(args[3]), std::stof(args[4]));
        float duration = std::stof(args[5]);
        
        // 设置传送门速度
        PhysicsState portal_physics;
        portal_physics.linear_velocity = velocity;
        portal_manager_->update_portal_physics_state(portal_id, portal_physics);
        
        std::cout << "Starting moving portal test:\n";
        std::cout << "  Portal: " << args[1] << "\n";
        std::cout << "  Velocity: (" << velocity.x << ", " << velocity.y << ", " << velocity.z << ")\n";
        std::cout << "  Duration: " << duration << " seconds\n";
        std::cout << "\nSimulating...\n";
        
        float time_step = 0.016f; // 60 FPS
        int total_steps = static_cast<int>(duration / time_step);
        int collision_count = 0;
        
        for (int step = 0; step < total_steps; ++step) {
            float current_time = step * time_step;
            
            // 检查是否有传送发生
            size_t teleports_before = portal_manager_->get_teleporting_entity_count();
            
            portal_manager_->update(time_step);
            
            size_t teleports_after = portal_manager_->get_teleporting_entity_count();
            
            if (teleports_after > teleports_before) {
                collision_count++;
                std::cout << "  [" << std::fixed << std::setprecision(3) << current_time 
                          << "s] Collision detected! Teleport triggered.\n";
            }
            
            // 每秒报告一次状态
            if (step % 60 == 0 && step > 0) {
                std::cout << "  [" << std::fixed << std::setprecision(1) << current_time 
                          << "s] Progress: " << (step * 100 / total_steps) << "%\n";
            }
        }
        
        // 停止传送门
        portal_physics.linear_velocity = Vector3(0, 0, 0);
        portal_manager_->update_portal_physics_state(portal_id, portal_physics);
        
        std::cout << "\nTest completed!\n";
        std::cout << "  Total collisions/teleports: " << collision_count << "\n";
        std::cout << "  Portal stopped.\n";
    }

    void PortalConsole::cmd_simulate(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            std::cout << "Usage: simulate <duration> [fps]\n";
            std::cout << "  Simulate physics for given duration in seconds\n";
            return;
        }
        
        float duration = std::stof(args[1]);
        float fps = 60.0f;
        
        if (args.size() > 2) {
            fps = std::stof(args[2]);
        }
        
        if (fps <= 0) {
            std::cout << "Invalid FPS value. Using default 60 FPS.\n";
            fps = 60.0f;
        }
        
        float time_step = 1.0f / fps;
        int total_steps = static_cast<int>(duration * fps);
        
        std::cout << "Starting simulation:\n";
        std::cout << "  Duration: " << duration << " seconds\n";
        std::cout << "  FPS: " << fps << " (" << time_step << "s per frame)\n";
        std::cout << "  Total frames: " << total_steps << "\n\n";
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int step = 0; step < total_steps; ++step) {
            portal_manager_->update(time_step);
            
            // 每10%进度报告一次
            if (step % (total_steps / 10) == 0 && step > 0) {
                float current_time = step * time_step;
                int progress = (step * 100) / total_steps;
                
                std::cout << "[" << std::fixed << std::setprecision(1) << current_time 
                          << "s] Progress: " << progress << "% - Teleporting entities: " 
                          << portal_manager_->get_teleporting_entity_count() << "\n";
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\nSimulation completed!\n";
        std::cout << "  Real time taken: " << duration_ms.count() << " ms\n";
        std::cout << "  Performance ratio: " << std::fixed << std::setprecision(2) 
                  << (duration * 1000.0f / duration_ms.count()) << "x real-time\n";
    }

    void PortalConsole::cmd_debug_collision(const std::vector<std::string>& args) {
        if (args.size() < 3) {
            std::cout << "Usage: debug_collision <entity> <portal>\n";
            std::cout << "  Show detailed collision detection information\n";
            return;
        }
        
        EntityId entity_id = find_entity_by_name(args[1]);
        PortalId portal_id = find_portal_by_name(args[2]);
        
        if (entity_id == INVALID_ENTITY_ID) {
            std::cout << "Entity '" << args[1] << "' not found.\n";
            return;
        }
        
        if (portal_id == INVALID_PORTAL_ID) {
            std::cout << "Portal '" << args[2] << "' not found.\n";
            return;
        }
        
        const Portal* portal = portal_manager_->get_portal(portal_id);
        Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
        PhysicsState entity_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);
        Vector3 bounds_min, bounds_max;
        interfaces_.physics_query->get_entity_bounds(entity_id, bounds_min, bounds_max);
        
        std::cout << "\n=== Collision Debug Information ===\n";
        std::cout << "Entity: " << args[1] << " (ID: " << entity_id << ")\n";
        std::cout << "  Position: (" << entity_transform.position.x << ", " 
                  << entity_transform.position.y << ", " << entity_transform.position.z << ")\n";
        std::cout << "  Velocity: (" << entity_physics.linear_velocity.x << ", " 
                  << entity_physics.linear_velocity.y << ", " << entity_physics.linear_velocity.z << ")\n";
        std::cout << "  Bounds: (" << bounds_min.x << ", " << bounds_min.y << ", " << bounds_min.z 
                  << ") to (" << bounds_max.x << ", " << bounds_max.y << ", " << bounds_max.z << ")\n";
        
        std::cout << "\nPortal: " << args[2] << " (ID: " << portal_id << ")\n";
        const PortalPlane& plane = portal->get_plane();
        std::cout << "  Center: (" << plane.center.x << ", " << plane.center.y << ", " << plane.center.z << ")\n";
        std::cout << "  Normal: (" << plane.normal.x << ", " << plane.normal.y << ", " << plane.normal.z << ")\n";
        std::cout << "  Size: " << plane.width << " x " << plane.height << "\n";
        
        const PhysicsState& portal_physics = portal->get_physics_state();
        std::cout << "  Velocity: (" << portal_physics.linear_velocity.x << ", " 
                  << portal_physics.linear_velocity.y << ", " << portal_physics.linear_velocity.z << ")\n";
        
        // 计算实体包围盒到传送门平面的距离
        Vector3 entity_center = entity_transform.position;
        float distance_to_plane = (entity_center - plane.center).dot(plane.normal);
        
        std::cout << "\nGeometry Analysis:\n";
        std::cout << "  Entity center to plane distance: " << distance_to_plane << "\n";
        
        // 检查实体是否在传送门范围内
        Vector3 relative_pos = entity_center - plane.center;
        float right_distance = std::abs(relative_pos.dot(plane.right));
        float up_distance = std::abs(relative_pos.dot(plane.up));
        
        bool in_portal_bounds = (right_distance <= plane.width * 0.5f) && (up_distance <= plane.height * 0.5f);
        
        std::cout << "  Right distance from portal center: " << right_distance << " (max: " << (plane.width * 0.5f) << ")\n";
        std::cout << "  Up distance from portal center: " << up_distance << " (max: " << (plane.height * 0.5f) << ")\n";
        std::cout << "  Entity in portal bounds: " << (in_portal_bounds ? "YES" : "NO") << "\n";
        
        // 检查是否应该触发传送
        bool should_teleport = false;
        if (distance_to_plane < -0.1f && in_portal_bounds) {
            should_teleport = true;
        }
        
        std::cout << "  Should trigger teleport: " << (should_teleport ? "YES" : "NO") << "\n";
        
        if (should_teleport && portal->is_linked()) {
            std::cout << "  -> Can teleport to portal ID: " << portal->get_linked_portal() << "\n";
        }
    }

    void PortalConsole::cmd_simulate_collision_detection(const std::vector<std::string>& args) {
        if (args.size() < 3) {
            std::cout << "Usage: simulate_collision_detection <duration> [fps]\n";
            std::cout << "  Simulate collision detection logic in console (engine role)\n";
            return;
        }
        
        float duration = std::stof(args[1]);
        float fps = 60.0f;
        
        if (args.size() > 2) {
            fps = std::stof(args[2]);
        }
        
        float time_step = 1.0f / fps;
        int total_steps = static_cast<int>(duration * fps);
        int collision_count = 0;
        
        std::cout << "Starting collision detection simulation:\n";
        std::cout << "  Duration: " << duration << " seconds\n";
        std::cout << "  FPS: " << fps << "\n";
        std::cout << "  Time step: " << time_step << "s\n\n";
        
        for (int step = 0; step < total_steps; ++step) {
            float current_time = step * time_step;
            
            // 模擬物理系統的碰撞檢測（引擎角色）
            for (const auto& [entity_id, entity_name] : entity_names_) {
                // 跳過正在傳送的實體（通過查詢 PortalManager）
                const TeleportState* teleport_state = portal_manager_->get_entity_teleport_state(entity_id);
                if (teleport_state && teleport_state->is_teleporting) {
                    continue;
                }
                
                Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
                PhysicsState entity_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);
                Vector3 bounds_min, bounds_max;
                interfaces_.physics_query->get_entity_bounds(entity_id, bounds_min, bounds_max);
                
                // 更新實體位置（模擬物理）
                entity_transform.position = entity_transform.position + entity_physics.linear_velocity * time_step;
                get_physics_query()->update_entity_transform(entity_id, entity_transform);
                
                // 檢查與所有傳送門的碰撞（引擎角色）
                for (const auto& [portal_id, portal_name] : portal_names_) {
                    const Portal* portal = portal_manager_->get_portal(portal_id);
                    if (!portal || !portal->is_active() || !portal->is_linked()) {
                        continue;
                    }
                    
                    // 檢查實體是否穿越傳送門（引擎的碰撞檢測邏輯）
                    if (check_entity_portal_crossing(entity_id, portal_id)) {
                        collision_count++;
                        std::cout << "  [" << std::fixed << std::setprecision(3) << current_time 
                                  << "s] Collision: " << entity_name << " -> " << portal_name << "\n";
                        
                        // 調用傳送門庫進行傳送（引擎調用庫）
                        const PhysicsState& portal_physics = portal->get_physics_state();
                        if (portal_physics.linear_velocity.length() > 0.001f) {
                            portal_manager_->teleport_entity_with_velocity(entity_id, portal_id, portal->get_linked_portal());
                        } else {
                            portal_manager_->teleport_entity(entity_id, portal_id, portal->get_linked_portal());
                        }
                        break;
                    }
                }
            }
            
            // 更新傳送門位置（如果有速度）
            for (const auto& [portal_id, portal_name] : portal_names_) {
                Portal* portal = portal_manager_->get_portal(portal_id);
                if (!portal) continue;
                
                const PhysicsState& portal_physics = portal->get_physics_state();
                if (portal_physics.linear_velocity.length() > 0.001f) {
                    PortalPlane plane = portal->get_plane();
                    plane.center = plane.center + portal_physics.linear_velocity * time_step;
                    portal_manager_->update_portal_plane(portal_id, plane);
                }
            }
            
            // 讓傳送門庫處理傳送狀態
            portal_manager_->update(time_step);
            
            // 進度報告
            if (step % (total_steps / 10) == 0 && step > 0) {
                int progress = (step * 100) / total_steps;
                std::cout << "  [" << std::fixed << std::setprecision(1) << current_time 
                          << "s] Progress: " << progress << "%\n";
            }
        }
        
        std::cout << "\nSimulation completed!\n";
        std::cout << "  Total collisions detected: " << collision_count << "\n";
        std::cout << "  Current teleporting entities: " << portal_manager_->get_teleporting_entity_count() << "\n";
    }

    bool PortalConsole::check_entity_portal_crossing(EntityId entity_id, PortalId portal_id) {
        // 這是引擎應該實現的碰撞檢測邏輯
        const Portal* portal = portal_manager_->get_portal(portal_id);
        if (!portal) return false;
        
        Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
        Vector3 bounds_min, bounds_max;
        interfaces_.physics_query->get_entity_bounds(entity_id, bounds_min, bounds_max);
        
        const PortalPlane& plane = portal->get_plane();
        Vector3 entity_center = entity_transform.position;
        
        // 檢查實體中心是否穿過傳送門平面
        float distance_to_plane = (entity_center - plane.center).dot(plane.normal);
        
        // 檢查是否在傳送門範圍內
        Vector3 relative_pos = entity_center - plane.center;
        float right_distance = std::abs(relative_pos.dot(plane.right));
        float up_distance = std::abs(relative_pos.dot(plane.up));
        
        bool in_portal_bounds = (right_distance <= plane.width * 0.5f) && (up_distance <= plane.height * 0.5f);
        
        // 簡單的穿越檢測：如果實體在傳送門後方且在範圍內
        return (distance_to_plane < -0.2f) && in_portal_bounds;
    }

    void PortalConsole::cmd_exit(const std::vector<std::string>& /*args*/) {
        std::cout << "Goodbye!\n";
        running_ = false;
    }

    std::vector<std::string> PortalConsole::split_command(const std::string& command) {
        std::vector<std::string> tokens;
        std::istringstream iss(command);
        std::string token;
        
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        return tokens;
    }

    PortalId PortalConsole::find_portal_by_name(const std::string& name) {
        for (const auto& [portal_id, portal_name] : portal_names_) {
            if (portal_name == name) {
                return portal_id;
            }
        }
        return INVALID_PORTAL_ID;
    }

    EntityId PortalConsole::find_entity_by_name(const std::string& name) {
        for (const auto& [entity_id, entity_name] : entity_names_) {
            if (entity_name == name) {
                return entity_id;
            }
        }
        return INVALID_ENTITY_ID;
    }

    void PortalConsole::print_banner() {
        std::cout << "\n";
        std::cout << "████████╗███████╗██╗     ███████╗██████╗  ██████╗ ██████╗ ████████╗\n";
        std::cout << "╚══██╔══╝██╔════╝██║     ██╔════╝██╔══██╗██╔═══██╗██╔══██╗╚══██╔══╝\n";
        std::cout << "   ██║   █████╗  ██║     █████╗  ██████╔╝██║   ██║██████╔╝   ██║   \n";
        std::cout << "   ██║   ██╔══╝  ██║     ██╔══╝  ██╔═══╝ ██║   ██║██╔══██╗   ██║   \n";
        std::cout << "   ██║   ███████╗███████╗███████╗██║     ╚██████╔╝██║  ██║   ██║   \n";
        std::cout << "   ╚═╝   ╚══════╝╚══════╝╚══════╝╚═╝      ╚═════╝ ╚═╝  ╚═╝   ╚═╝   \n";
        std::cout << "\n";
        std::cout << "        ██████╗ ██████╗ ██████╗ ████████╗ █████╗ ██╗     \n";
        std::cout << "        ██╔══██╗██╔═══██╗██╔══██╗╚══██╔══╝██╔══██╗██║     \n";
        std::cout << "        ██████╔╝██║   ██║██████╔╝   ██║   ███████║██║     \n";
        std::cout << "        ██╔═══╝ ██║   ██║██╔══██╗   ██║   ██╔══██║██║     \n";
        std::cout << "        ██║     ╚██████╔╝██║  ██║   ██║   ██║  ██║███████╗\n";
        std::cout << "        ╚═╝      ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚══════╝\n";
        std::cout << "\n";
        std::cout << "             ╔═══════════════════════════════════════╗\n";
        std::cout << "             ║    Portable Portal Console v1.0.0    ║\n";
        std::cout << "             ║         Type 'help' to start         ║\n";
        std::cout << "             ╚═══════════════════════════════════════╝\n";
    }

    void PortalConsole::print_portal_info(const Portal* portal, const std::string& name) {
        const PortalPlane& plane = portal->get_plane();
        std::cout << "  Portal '" << name << "' (ID: " << portal->get_id() << ")\n";
        std::cout << "    Position: (" << std::fixed << std::setprecision(2) 
                  << plane.center.x << ", " << plane.center.y << ", " << plane.center.z << ")\n";
        std::cout << "    Normal: (" << plane.normal.x << ", " << plane.normal.y << ", " << plane.normal.z << ")\n";
        std::cout << "    Size: " << plane.width << " x " << plane.height << "\n";
        std::cout << "    Active: " << (portal->is_active() ? "YES" : "NO") << "\n";
        std::cout << "    Linked: " << (portal->is_linked() ? "YES" : "NO");
        if (portal->is_linked()) {
            std::cout << " (Portal ID: " << portal->get_linked_portal() << ")";
        }
        std::cout << "\n";
        std::cout << "    Recursive: " << (portal->is_recursive() ? "YES" : "NO") << "\n";
    }

    void PortalConsole::print_entity_info(EntityId entity_id, const std::string& name) {
        Transform transform = interfaces_.physics_query->get_entity_transform(entity_id);
        PhysicsState physics = interfaces_.physics_query->get_entity_physics_state(entity_id);
        
        std::cout << "  Entity '" << name << "' (ID: " << entity_id << ")\n";
        std::cout << "    Position: (" << std::fixed << std::setprecision(2) 
                  << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << ")\n";
        std::cout << "    Velocity: (" << physics.linear_velocity.x << ", " 
                  << physics.linear_velocity.y << ", " << physics.linear_velocity.z << ")\n";
        std::cout << "    Mass: " << physics.mass << "\n";
    }

} // namespace Console
} // namespace Portal
