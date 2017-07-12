#include "scenario_controller.hpp"

ScenarioController::ScenarioController() {}
ScenarioController::~ScenarioController() {}
void ScenarioController::execute() {}

void ScenarioController::initialize_node_fb() {}

void ScenarioController::set_sc_timeout_ms(float t){
  sc_timeout_ms = t;
}

void ScenarioController::set_node_parameter(int node, char cont_type, void* _arg){
  char cont_msg[16];
  int arg_len = get_control_arg_len(cont_type);

  cont_msg[0] = CRTS_MSG_CONTROL;
  cont_msg[1] = cont_type;
  if(arg_len > 0)
    memcpy((void*)&cont_msg[2], _arg, arg_len);
        
  if (node > sp.num_nodes) {
    printf("set_node_parameters() was called for a node which exceeds the number of nodes in this scenario\n");
    exit(1);
  } else {
    write(TCP_nodes[node-1], cont_msg, 2+arg_len);
  }
}

void ScenarioController::receive_feedback(int node, char fb_type, void* _arg){
  pthread_mutex_lock(&sc_mutex);
  sc_event = FEEDBACK;
  fb.node = node+1;
  fb.fb_type = fb_type;
  fb.arg = _arg;
  execute();
  pthread_mutex_unlock(&sc_mutex);
}

void ScenarioController::start_sc() {
  
  // start sc worker thread
  sc_running = false;       // ce is not running initially
  sc_thread_running = true; // ce thread IS running initially
  pthread_mutex_init(&sc_mutex, NULL);
  pthread_cond_init(&sc_execute_sig, NULL);
  pthread_cond_init(&sc_cond, NULL); // cognitive engine condition
  pthread_create(&sc_process, NULL, sc_worker, (void *)this);

  // set ce running flag
  sc_running = true;
  
  // wait to ensure thread is ready for signal
  usleep(1e4);

  // signal condition for the ce to start listening for events of interest
  pthread_cond_signal(&sc_cond);
}

void ScenarioController::stop_sc() {
  // reset ce running flag
  sc_running = false;

  // signal condition (tell sc worker to continue)
  sc_thread_running = false;
  pthread_cond_signal(&sc_cond);

  void *sc_exit_status;
  pthread_join(sc_process, &sc_exit_status);

  pthread_mutex_destroy(&sc_mutex);
  pthread_cond_destroy(&sc_cond);
}

// main loop of SC
void *sc_worker(void *_arg) {
  
  ScenarioController *SC = (ScenarioController *)_arg;

  struct timeval time_now;
  double timeout_ns;
  double timeout_spart;
  double timeout_nspart;
  struct timespec timeout;

  // until SC thread is joined
  while (SC->sc_thread_running) {

    pthread_mutex_lock(&SC->sc_mutex);
    pthread_cond_wait(&SC->sc_cond, &SC->sc_mutex);
    pthread_mutex_unlock(&SC->sc_mutex);

    while (SC->sc_running) {

      // Get current time of day
      gettimeofday(&time_now, NULL);

      // Calculate timeout time in nanoseconds
      timeout_ns = (double)time_now.tv_usec * 1e3 +
                   (double)time_now.tv_sec * 1e9 + SC->sc_timeout_ms * 1e6;
      // Convert timeout time to s and ns parts
      timeout_nspart = modf(timeout_ns / 1e9, &timeout_spart);
      // Put timeout into timespec struct
      timeout.tv_sec = (long int)timeout_spart;
      timeout.tv_nsec = (long int)(timeout_nspart * 1e9);

      // Wait for signal. For now there are only two event types, so
      // if not a timeout assume we've received feedback
      pthread_mutex_lock(&SC->sc_mutex);
      if (ETIMEDOUT == pthread_cond_timedwait(&SC->sc_execute_sig,
                                              &SC->sc_mutex, &timeout)){
        SC->sc_event = TIMEOUT;
      }
      
      // execute SC
      SC->execute();
      pthread_mutex_unlock(&SC->sc_mutex);
    }
  }
  pthread_exit(NULL);
}

