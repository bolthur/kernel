/**
 * Copyright (C) 2018 - 2026 bolthur project.
 *
 * This file is part of bolthur/kernel.
 *
 * bolthur/kernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bolthur/kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bolthur/kernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "queue.h"
#include "thread.h"
#include "../lib/inttypes.h"
#include "../lib/stdlib.h"
#if defined( PRINT_PROCESS )
  #include "../debug/debug.h"
#endif

static queue_manager_t* queue_manager;

/**
 * @fn int32_t queue_compare_vruntime_callback(const avl_node_t*, const avl_node_t*)
 * @brief Compare vruntime callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t queue_compare_vruntime_callback(
  const avl_node_t* a,
  const avl_node_t* b
) {
  auto const thread_a = TASK_THREAD_GET_QUEUE_BLOCK( a );
  auto const thread_b = TASK_THREAD_GET_QUEUE_BLOCK( b );
  if ( thread_a->vruntime < thread_b->vruntime ) {
    return -1;
  }
  if ( thread_a->vruntime > thread_b->vruntime ) {
    return 1;
  }
  if ( thread_a->id < thread_b->id ) {
    return -1;
  }
  if ( thread_a->id > thread_b->id ) {
    return 1;
  }
  // equal => return 0
  return 0;
}

/**
 * @fn bool task_queue_init(void)
 * @brief Initialize task process manager
 *
 * @return queue_manager_t*
 */
 bool task_queue_init( void ) {
  queue_manager = malloc( sizeof( *queue_manager ) );
  if ( ! queue_manager ) {
    return false;
  }
  queue_manager->thread_scheduling_tree = avl_create_tree( queue_compare_vruntime_callback, nullptr, nullptr );
  if ( ! queue_manager->thread_scheduling_tree ) {
    free( queue_manager );
    return false;
  }
  queue_manager->thread_wait_queue = list_construct( nullptr, nullptr, nullptr );
  if ( ! queue_manager->thread_wait_queue ) {
    avl_destroy_tree( queue_manager->thread_scheduling_tree );
    free( queue_manager );
    return false;
  }
  queue_manager->min_vruntime = 0;
  queue_manager->thread_count = 0;
  return true;;
}

/**
 * @fn void task_queue_destroy(void)
 * @brief Destroy task queue
 */
void task_queue_destroy( void ) {
  if ( ! queue_manager ) {
    return;
  }
  if ( queue_manager->thread_scheduling_tree ) {
    avl_destroy_tree( queue_manager->thread_scheduling_tree );
  }
  if ( queue_manager->thread_wait_queue ) {
    list_destruct( queue_manager->thread_wait_queue );
  }
  free( queue_manager );
}

/**
 * @fn task_thread_t* task_queue_dequeue(void)
 * @brief Wrapper to dequeue next task
 * @return
 */
task_thread_t* task_queue_dequeue( void ) {
  if ( ! queue_manager || ! queue_manager->thread_scheduling_tree ) {
    return nullptr;
  }
  // get min node
  avl_node_t* node = avl_get_min( queue_manager->thread_scheduling_tree->root );
  // handle nothing
  if ( ! node ) {
    return nullptr;
  }
  // remove it
  avl_remove_by_node( queue_manager->thread_scheduling_tree, node );
  // increment count
  queue_manager->thread_count--;
  // return thread block
  return TASK_THREAD_GET_QUEUE_BLOCK( node );
}

/**
 * @fn task_thread_t* task_queue_peek(void)
 * @brief Peek next queue entry
 * @return
 */
task_thread_t* task_queue_peek( void ) {
  if ( ! queue_manager || ! queue_manager->thread_scheduling_tree ) {
    return nullptr;
  }
  // get min node
  avl_node_t* node = avl_get_min( queue_manager->thread_scheduling_tree->root );
  // handle nothing
  if ( ! node ) {
    return nullptr;
  }
  // return thread block
  return TASK_THREAD_GET_QUEUE_BLOCK( node );
}

/**
 * @fn void task_queue_dequeue_specific(task_thread_t*)
 * @brief Dequeue speicifc thread
 * @param thread
 */
void task_queue_dequeue_specific( task_thread_t* thread ) {
  if ( ! queue_manager || ! queue_manager->thread_scheduling_tree || ! thread ) {
    return;
  }
  // remove node
  avl_remove_by_node( queue_manager->thread_scheduling_tree, &thread->queue_node );
  // increment count
  queue_manager->thread_count--;
}

/**
 * @fn void task_queue_enqueue(task_thread_t*)
 * @brief Function enqueue a task
 * @param thread
 */
void task_queue_enqueue( task_thread_t* thread ) {
  if ( ! queue_manager || ! queue_manager->thread_scheduling_tree || ! thread ) {
    return;
  }
  // prepare queue node
  avl_prepare_node( &thread->queue_node, thread->vruntime );
  // add to tree
  avl_insert_by_node( queue_manager->thread_scheduling_tree, &thread->queue_node );
  // increment count
  queue_manager->thread_count++;
}

/**
 * @fn void task_queue_enqueue_blocked(task_thread_t*)
 * @brief Queue blocked thread
 * @param thread
 */
void task_queue_enqueue_blocked( task_thread_t* thread ) {
  if ( ! queue_manager || ! queue_manager->thread_wait_queue || ! thread ) {
    return;
  }
  // push to blocked
  list_push_back_data( queue_manager->thread_wait_queue, thread );
  // remove from scheduling tree
  avl_remove_by_node( queue_manager->thread_scheduling_tree, &thread->queue_node );
}

/**
 * @fn void task_queue_dequeue_blocked(task_thread_t*)
 * @brief Dequeue specific blocked
 * @param thread
 */
void task_queue_dequeue_blocked( task_thread_t* thread ) {
  if ( ! queue_manager || ! queue_manager->thread_wait_queue || ! thread ) {
    return;
  }
  // remove from blocked list
  list_remove_data( queue_manager->thread_wait_queue, thread, true );
  // adjust vruntime for fair scheduling
  if ( thread->vruntime < queue_manager->min_vruntime ) {
    // inactivity bonus
    constexpr uint64_t bonus = 10000;
    // adjust vruntime
    thread->vruntime = queue_manager->min_vruntime > bonus ? queue_manager->min_vruntime - bonus : 0;
  }
}

/**
 * @fn list_item_t* task_queue_get_first_blocked( void )
 * @brief Function to get first blocked thread
 * @return
 */
list_item_t* task_queue_get_first_blocked( void ) {
  if ( ! queue_manager || ! queue_manager->thread_wait_queue ) {
    return nullptr;
  }
  return queue_manager->thread_wait_queue->first;
}

/**
 * @fn void task_queue_update_min_vruntime(task_thread_t*)
 * @brief update min vruntime
 * @param current_thread
 */
void task_queue_update_min_vruntime( task_thread_t* current_thread ) {
  if ( ! queue_manager || ! queue_manager->thread_wait_queue ) {
    return;
  }
  // use from manager
  uint64_t vruntime = queue_manager->min_vruntime;
  // handle thread passed
  if ( current_thread ) {
    vruntime = current_thread->vruntime;
  }
  // peek min thread
  auto const min = task_queue_peek();
  // handle existing
  if ( min ) {
    // overwrite vruntime in case no current thread is passed
    if ( ! current_thread ) {
      vruntime = min->vruntime;
    // use minimum of vruntime and min if current thread is set
    } else {
      vruntime = vruntime < min->vruntime
        ? vruntime
        : min->vruntime;
    }
  }
  // overwrite min vruntime
  queue_manager->min_vruntime = queue_manager->min_vruntime > vruntime
    ? queue_manager->min_vruntime
    : vruntime;
}
