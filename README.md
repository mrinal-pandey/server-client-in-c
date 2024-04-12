A simple Client-Server Program in C

Details:
- each user is served by a seperate pthread
- user command ‘ADD 10’ adds 10 to the linked list on the server
- user command ‘DEL 10’ deletes number 10 from the linked list on the server
- user command ‘SHOW’ shows all elements in the list on the server
- user can not add duplicate number in the list
- multiple users can run ADD and DEL the element to a single linked list without concurrency issues
- the client can connect from different host or machine providing IP address of server in client.c file
