#include "linkedlist.h"
#include "parsecmd.h"

linked_list_t *ll_create()
{
  linked_list_t *list = calloc(sizeof(linked_list_t), 1);
  list->head = NULL;
  list->tail = NULL;
  list->length = 0;
  return list;
}

void ll_destroy(linked_list_t *list)
{
  if (list == NULL)
    return;

  ll_node_t *curr = list->head;
  ll_node_t *next = NULL;
  while (curr != NULL)
  {
    next = curr->next;
    free(curr);
    curr = next;
  }
  free(list);
}

ll_node_t *ll_create_node(void *object)
{
  ll_node_t *node = calloc(sizeof(ll_node_t), 1);
  node->next = NULL;
  node->prev = NULL;
  node->object = object;
  return node;
}

ll_node_t *ll_add(linked_list_t *list, void *object)
{
  if (list == NULL || object == NULL)
    return NULL;

  ll_node_t *node = ll_create_node(object);
  /* List is empty. */
  if (list->head == NULL)
  {
    list->head = node;
    list->tail = node;
  }

  /* List has one or more elements. */
  else
  {
    list->tail->next = node;
    node->prev = list->tail;
    list->tail = node;
  }

  list->length++;
  return node;
}

ll_node_t *ll_add_front(linked_list_t *list, void *object)
{
  if (list == NULL || object == NULL)
    return NULL;

  ll_node_t *node = ll_create_node(object);
  /* List is empty. */
  if (list->head == NULL)
  {
    list->head = node;
    list->tail = node;
  }

  /* List has one or more elements. */
  else
  {
    node->next = list->head;
    list->head->prev = node;
    list->head = node;
  }

  list->length++;
  return node;
}

ll_node_t *ll_add_after(linked_list_t *list, ll_node_t *node, void *object)
{
  if (list == NULL || node == NULL || object == NULL)
    return NULL;

  ll_node_t *new_node = ll_create_node(object);
  /* Update pointers. */
  new_node->prev = node;
  new_node->next = node->next;
  if (node->next != NULL)
    node->next->prev = new_node;
  node->next = new_node;

  /* Added to end of list. */
  if (node == list->tail)
    list->tail = new_node;

  list->length++;
  return new_node;
}

void *ll_remove(linked_list_t *list, ll_node_t *node)
{
  if (list == NULL || node == NULL)
    return NULL;
  void *object = node->object;

  /* Update linked list pointers. */
  if (node == list->head)
    list->head = node->next;
  else
    node->prev->next = node->next;

  if (node == list->tail)
    list->tail = node->prev;
  else
    node->next->prev = node->prev;

  /* Free memory. */
  free(node);
  list->length--;

  return object;
}

ll_node_t *ll_find(linked_list_t *list, void *object)
{
  if (list == NULL || object == NULL)
    return NULL;

  ll_node_t *curr = list->head;
  while (curr != NULL)
  {
    if (curr->object == object)
    {
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

ll_node_t *ll_find_pid(linked_list_t *list, pid_t pid, linked_list_t *list_done)
{
  if (list == NULL)
    return NULL;

  struct background_process *bgprocess_tmp = NULL;

  ll_node_t *curr = list->head;
  while (curr != NULL)
  {
    bgprocess_tmp = (struct background_process *)curr->object;
    if (bgprocess_tmp->pid == pid)
    {
      bgprocess_tmp->status = TERMINATE;
#ifdef DEBUG
      printf("****** process with pid: %d and cmd: %s done! ******\n", bgprocess_tmp->pid, bgprocess_tmp->cmd);
#endif
      ll_add_front(list_done, (struct background_process *)bgprocess_tmp);
      ll_remove(list, curr);
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

ll_node_t *ll_front(linked_list_t *list)
{
  return list->head;
}

ll_node_t *ll_back(linked_list_t *list)
{
  return list->tail;
}

unsigned int ll_length(linked_list_t *list)
{
  return list->length;
}

char *get_process_state(int status)
{
  char *state = "";
  switch (status)
  {
  case PENDING:
    state = "Pending";
    break;
  case RUNNING:
    state = "Running";
    break;
  case STOPPED:
    state = "Stopped";
    break;
  case TERMINATE:
    state = "Done";
    break;
  default:
    break;
  }
  return state;
}

void print_bgprocess_list(linked_list_t *list)
{
  ll_node_t *tmp = NULL;
  struct background_process *bgprocess = NULL;
  int first = 0;
  int index = 1;
  char *status;

  if (ll_length(list) < 1)
    return;

  // Return if list is empty
  if (list == NULL)
  {
    printf("List is empty.");
    return;
  }

  tmp = list->head;

  while (tmp != NULL)
  {
    bgprocess = NULL;
    bgprocess = (struct background_process *)tmp->object;
    // printf("bg cmd address: %p\n", bgprocess);
    if (first == 0)
      status = "+";
    else
      status = "-";
    printf("[%d]%s {pid = %d}, {status=%s}, {cmd=%s}\n", index, status, bgprocess->pid, get_process_state(bgprocess->status), bgprocess->cmd); // Print data of current node
    tmp = tmp->next;                                                                                                                           // Move to next node
    first = 1;
    index++;
  }
}

void freeList(linked_list_t *list)
{
  ll_node_t *tmp = NULL;
  if (ll_length(list) < 1)
    return;
  ll_node_t *head = list->head;

  while (head != NULL)
  {
    tmp = NULL;
    tmp = head;
    head = head->next;
    free(tmp);
  }

  list->head = list->tail = NULL;
  list->length = 0;
}
