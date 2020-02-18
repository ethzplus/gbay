from __future__ import print_function

PY_NODATA_VALUE = 255  # Same as in embed_python.h. To communicate a NODATA value from C and to C the likelihood has to have this value for every state.
PY_DISCRETE = 0   # Same as in embed_python.h. By default nodes are considered DISCRETE
PY_CONTINUOUS = 1 # Same as in embed_python.h

# Returns the node which name is 'name' from the list of nodes 'node_list'
def getNodeByName(node_list, name):
	for i, node in enumerate(node_list):
		if (node['name'] == name):
			return node
	return None

def getState(likelihood, state):  # for discrete node
	if (('type' in node) and (node['type'] == PY_CONTINUOUS)):
		print("getState: ERROR: node", node['name'], "is not DISCRETE")
		return;

	return likelihood[state]

def getLikelihood(node, cell):
	if (('type' in node) and (node['type'] == PY_CONTINUOUS)):
		print("getLikelihood: ERROR: node", node['name'], "is not DISCRETE")
		return;

	return node['data'][cell]

def setLikelihood(node, cell, likelihood):
	if (('type' in node) and (node['type'] == PY_CONTINUOUS)):
		print("setLikelihood: ERROR: node", node['name'], "is not DISCRETE")
		return;
		
	node['data'][cell] = likelihood[:]

def setEvidence(node, cell, evidence):
	if (('type' in node) and (node['type'] == PY_CONTINUOUS)):
		print("setEvidence: ERROR: node", node['name'], "is not DISCRETE")
		return;

	likelihood =  getLikelihood(node, cell)[:]
	for i in range(len(likelihood)):
		if (i==evidence):
			likelihood[i] = 100
		else:
			likelihood[i] = 0

	node['data'][cell] = likelihood

def getValue(node, cell):   # for continuous node
	if ((not 'type' in node) or (node['type'] != PY_CONTINUOUS)):
		print("getValue: ERROR: node", node['name'], "is not CONTINUOUS")
		return;

	return node['data'][cell]

def setValue(node, cell, value):   # for continuous node
	if ((not 'type' in node) or (node['type'] != PY_CONTINUOUS)):
		print("setValue: ERROR: node", node['name'], "is not CONTINUOUS")
		return;

	node['data'][cell] = value

def getStateCell(node, cell, state):
	return node['data'][cell][state]

def printNode(node):
	print(node['name'])
	for cell in range(len(node['data'])):
		print(cell, getLikelihood(node, cell))

def newNode(name, n_cells, nodetype=PY_DISCRETE):
	node = {'name': name, 'data' : [], 'type': nodetype }
	for i in range(n_cells):
		node['data'].append([])
	return node

def copyLikelihood(node, cell, likelihood):
	node['data'][cell] = likelihood[:]

def getStateHighestLikelihood(node, cell):
	if isNODATA(node, cell):
		return -1
	return node['data'][cell].index(max(node['data'][cell]))

def getStateLowestLikelihood(node, cell):
	if isNODATA(node, cell):
		return -1
	return node['data'][cell].index(min(node['data'][cell]))

def isNODATA(node, cell):

	if (node['type'] == PY_CONTINUOUS ): # PY_CONTINUOUS from python_c.py
		return (getValue(node, cell) == PY_NODATA_VALUE)
	else:
		return (not getLikelihood(node, cell)) #check for empty likelihood

	# for i in getLikelihood(node, cell):
	# 	if i != PY_NODATA_VALUE:
	# 		return False
	# return True

# Function to check if the process function ran as expected for a given cell
def printLikelihoods(node_input_list, node_output_list, cell):

	print("Cell", cell)
	print("Input:")
	for node in node_input_list:
		print("\t", node['name'], getLikelihood(node, cell))
	print("Output:")
	for node in node_output_list:
		print("\t", node['name'], getLikelihood(node, cell))

def validResultData(node_list, total_cells, print_error=True):

	if (not(isinstance(node_list, list))):
		if (print_error):
			print("validResultData: ERROR: node_list is not of type 'list'")
		return False

	for node in node_list:

		if (not(isinstance(node, dict))):
			if (print_error):
				print("validResultData: ERROR: node is not of type 'dictionary'")
			return False

		if (not('name' in node)):
			if (print_error):
				print("validResultData: ERROR: node has no key named 'name'")
			return False

		print("validResultData:", node['name'])

		if (not('data' in node)):
			if (print_error):
				print("validResultData: ERROR: node has no key named 'data'")
			return False

		# check missing key 'type'
		if (not('type' in node)):
			print("validResultData: WARN: node has no key named 'type'. Automatically set to DISCRETE")
			node['type'] = PY_DISCRETE

		if (not(isinstance(node['name'], str)) and not (isinstance(node['name'], unicode)) ):
			if (print_error):
				print("validResultData: ERROR: node: ", node['name'], ": 'name' is not of type 'str' or 'unicode'")
			return False

		if (not(isinstance(node['data'], list))):
			if (print_error):
				print("validResultData: ERROR: node: ", node['name'], ": 'data' is not of type 'list'")
			return False

		if (len(node["data"]) != total_cells):
			print("validResultData: ERROR: Length of data of result node (", len(node["data"]) ,") should be the same as the number of cells/features (", total_cells, ")")
			return False

		printed = False

		nodata_counter = 0

		for cell_index, cell in enumerate(node['data']):

			if (not isNODATA(node, cell_index)):  # skip cells that mean NODATA

				if (node['type']==PY_DISCRETE): # DISCRETE. Defined in python_c.py

					total_prob = 0

					for state_prob in cell:
						total_prob += state_prob

					if (total_prob != 100):
						if (print_error):
							print("validResultData: ERROR: Likelihood sum is:", total_prob, ", not 100 in cell", cell_index, cell)
						return False

				elif (node['type']==PY_CONTINUOUS): # CONTINUOUS. Defined in python_c.py
					node['data'][cell_index] = float(node['data'][cell_index])

			else:
				nodata_counter +=1;

		if (nodata_counter == total_cells):
			print("validResultData: ERROR: All cells are NODATA")
			return False
			
	return True
