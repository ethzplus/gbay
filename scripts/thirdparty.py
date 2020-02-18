"""
Function to be implemented by third-party users

GDALDatasetH dataset: reference Dataset as returned by gdal.Open 
- http://www.gdal.org/gdal_tutorial.html
- http://gdal.org/python/osgeo.gdal.Dataset-class.html)

nodes_data: list of python dictionaries in the form {name: <string>nodename, states: <int>number_of_states, data: <list><bytearray>nodedata}

	data is a bytearray list of cell beliefs

	I.e. to get cell CELL belief of state STATE: 
		value = node.data[CELL][STATE]


return value: same as nodes_data: nodes with modified likelihood for the next iteration
"""

def process(dataset, nodes_data):


	# print 'Driver: ', dataset.GetDriver().ShortName,'/', \
	#       dataset.GetDriver().LongName
	# print 'Size is ',dataset.RasterXSize,'x',dataset.RasterYSize, \
	#       'x',dataset.RasterCount
	# print 'Projection is ',dataset.GetProjection()
	#geotransform = dataset.GetGeoTransform()
	# if not geotransform is None:
	#     print 'Origin = (',geotransform[0], ',',geotransform[3],')'
	#     print 'Pixel Size = (',geotransform[1], ',',geotransform[5],')'


	print "process: dataset: ",dataset
	print "process: nodes_data: ",nodes_data

	for i, node in enumerate(nodes_data):
		print i, node['name'], node['states'], node['data']


	new_nodes_data = [{'name' : 'water', 'data': []}]

	for cell in range(dataset.RasterXSize * dataset.RasterYSize ):

		new_nodes_data[0]['data'].append([])

		# for state in range(5):
		
		new_nodes_data[0]['data'][cell].append(bytearray('12'))
		

	# new_nodes_data = [{'name' : 'return_node', 'data': [bytearray('dddd')] }]


	# new_nodes_data = [{'name' : 'water', 'data': [bytearray('dddd')] }]

	print "process: new_nodes_data", new_nodes_data

	return new_nodes_data

def printHello():
	print 'Hello!'