from threading import Semaphore
from threading import Thread
from threading import Event
from threading import Lock
"""
    This module represents a cluster's computational node.

    Computer Systems Architecture course
    Assignment 1 - Cluster Activity Simulation
    march 2013
    
    Nume: 	Olteanu Vasilica - Claudiu
    Grupa: 	332CA
    
"""

syncComm = Lock()
class Node:
    """
        Class that represents a cluster node with computation and storage functionalities.
    """

    def __init__(self, node_ID, block_size, matrix_size, data_store):
        """
            Constructor.

            @param node_ID: a pair of IDs uniquely identifying the node; 
            IDs are integers between 0 and matrix_size/block_size
            @param block_size: the size of the matrix blocks stored in this node's datastore
            @param matrix_size: the size of the matrix
            @param data_store: reference to the node's local data store
        """
        self.node_ID = node_ID;
        self.block_size = block_size;
        self.matrix_size = matrix_size;
        self.data_store = data_store;
        self.sem = Semaphore(value=self.data_store.get_max_pending_requests(self))
        self.threads = []
        self.communicationTh = CommunicationThread(self)
        self.communicationTh.start()
        self.computeTh = ComputeThread(self)
        self.computeTh.start()
        self.syncCompute = Lock()


    def set_nodes(self, nodes):
        """
            Informs the current node of the other nodes in the cluster. 
            Guaranteed to be called before the first call to compute_matrix_block.

            @param nodes: a list containing all the nodes in the cluster
        """
        self.nodes = nodes;

    def compute_matrix_block(self, start_row, start_column, num_rows, num_columns):
        """
            Computes a given block of the result matrix.
            The method invoked by FEP nodes.

            @param start_row: the index of the first row in the block
            @param start_column: the index of the first column in the block
            @param num_rows: number of rows in the block
            @param num_columns: number of columns in the block

            @return: the block of the result matrix encoded as a row-order list of lists of integers

        """
        result = []
        with self.syncCompute:
            self.computeTh.set_values(start_row, start_column, num_rows, num_columns)
            self.computeTh.event.set()
            self.computeTh.semC.acquire()
            result = self.computeTh.get_result()

        return result

    def shutdown(self):
        """
            Instructs the node to shutdown (terminate all threads).
        """
        self.communicationTh.shut_down()
        self.computeTh.shut_down()

    def correct_IDs(self, i, j):
        """
            Verifica daca nod-ul curent are indicii are ID-ul (i, j)

            @param i: numarul liniei de inceput
            @param j: numarul coloanei de inceput

            @return: True daca nodul curent are ID-ul (i, j), False in caz contrat

        """
        if (self.node_ID[0] == i and self.node_ID[1] == j):
            return True
        else:
            return False


class ComputeThread(Thread):
    """
        Clasa ce mosteneste un thread si calculeaza rezultatul unui block.
    """

    def __init__(self, node):
        """
            Constructor.

            @param node: nod-ul caruia i se calculeaza bloc-ul 
        """
        Thread.__init__(self)
        self.node = node
        self.result = []
        self.semC = Semaphore(value=0)
        self.event = Event()
        self.event.clear()
        self.shutdown = False

    def run(self):
        """
            Calculeaza blocul [start_row:start_row + num_rows, start_column:start_column + num_columns]
            si anunta nodul cand acesta este gata, cu ajutorul semaforului semC.
        """
        self.node.data_store.register_thread(self.node)
        while True:
            while not self.event.isSet():
                self.event.wait()
            
            if self.shutdown:
                break

            for i in range(self.num_rows):
                row = []
                for j in range(self.num_columns):
                    value = 0
                    for k in range(self.node.matrix_size):
                        value = value + (self.get_element(self.start_row + i, k, "A") * 
                                        self.get_element(k, self.start_column + j, "B"))
                    row.append(value)
                self.result.append(row)

            self.event.clear()
            self.semC.release()

    def shut_down(self):
        """
            Anunta thread-ul ca va trebui sa se inchida.
        """
        
        self.shutdown = True
        self.event.set()

    def set_values(self, start_row, start_column, num_rows, num_columns):
        """
            Seteaza dimensiunile blocului ce va fi calculat.

            @param start_row: linia de inceput
            @param start_column: coloana de inceput
            @param num_rows: numarul de linii ce trebuiesc calculate
            @param num_columns: numarul de coloane ce trebuiesc calculate
        """
        self.start_row = start_row
        self.start_column = start_column
        self.num_rows = num_rows
        self.num_columns = num_columns
        self.result = []

    def get_element(self, row, column, matrix):
        """
            Determina nodul din care face parte elementul. Daca acesta este nodul sursa al thread-ului,
            se determina direct elementul. Daca nu, se face o cerere la threadul de comunicare al nodului
            si se asteapta pana cand acesta il anunta ca a determinat elementul.

            @param row: linia elementului
            @param column: coloana elementului
            @param matrix: matricea din care face parte

            @return: elementul de pe [row, column]
        """
        i = int(row / self.node.block_size)
        j = int(column / self.node.block_size)
        value = 0

        for node in self.node.nodes:
            if node.correct_IDs(i, j):
                pos = [row - i * self.node.block_size, column - j * self.node.block_size]
                if node == self:
                    if(matrix == "A"):
                        self.node.sem.acquire()
                        value = self.node.data_store.get_element_from_a(self.node, self.pos[0], self.pos[1])
                        self.node.sem.release()
                    elif(matrix == "B"):
                        self.node.sem.acquire()
                        value = self.node.data_store.get_element_from_b(self.node, self.pos[0], self.pos[1])
                        self.node.sem.release()
                else:
                    with syncComm:
                        node.communicationTh.set_values(pos, matrix)
                        node.communicationTh.event.set()
                        node.communicationTh.semC.acquire()
                        value = node.communicationTh.get_value()
                break

        return value

    def get_result(self):
        """
            @return: blocul calculat
        """
        
        return self.result




class CommunicationThread(Thread):
    """
        Clasa ce mosteneste un thread si este folosit pentru a determina elementele ce apartin unui nod.
    """

    def __init__(self, node):
        """
            Constructor.

            @param node: nod-ul parinte pentru care se determina elementele din datastore.
        """
        Thread.__init__(self)
        self.node = node
        self.pos = None
        self.matrix = None
        self.value = None
        self.shutdown = False
        self.event = Event()
        self.event.clear()
        self.semC = Semaphore(value=0)

    def set_values(self, pos, matrix):
        """
            @param pos: pozitia nodului ce trebuie determinat
            @param matrix: matricea din care face parte
        """
        self.pos = pos
        self.matrix = matrix

    def shut_down(self):
        """
            Anunta thread-ul de comunicatie ca nu mai are nevoie de alte elemente si se poate inchide.
        """
        self.shutdown = True
        self.event.set()

    def get_value(self):
        """
            @return: valoarea ce trebuia determinata
        """
        if(self.value is not None):     
            return self.value
        else:
            print "Value is still None"
            return 0

    def run(self):
        """
            Se inregistreaza thread-ul la datastore, apoi se asteapta aparitia unui eveniment. Daca evenimentul este
            inchidere se termina thread-ul, daca nu, se determina elementul din datastore.
        """
        self.node.data_store.register_thread(self.node)
        while True:
            while not self.event.isSet():
                self.event.wait()

            if self.shutdown:
                break

            if(self.matrix == "A"):
                self.node.sem.acquire()
                self.value = self.node.data_store.get_element_from_a(self.node, self.pos[0], self.pos[1])
                self.node.sem.release()
            elif(self.matrix == "B"):
                self.node.sem.acquire()
                self.value = self.node.data_store.get_element_from_b(self.node, self.pos[0], self.pos[1])
                self.node.sem.release()
            else:
                print "Matrix ", self.matrix, " doesn't exist"
            self.event.clear()
            self.semC.release()
