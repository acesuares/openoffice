#**************************************************************
#  
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#  
#    http://www.apache.org/licenses/LICENSE-2.0
#  
#  Unless required by applicable law or agreed to in writing,
#  software distributed under the License is distributed on an
#  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
#  specific language governing permissions and limitations
#  under the License.
#  
#**************************************************************



package installer::windows::removefile;

use installer::files;
use installer::globals;
use installer::windows::idtglobal;

########################################################################
# Returning the FileKey for a folderitem for removefile table.
########################################################################

sub get_removefile_filekey
{
	my ($folderitem) = @_;
	
	# returning the unique identifier
	
	my $identifier = "remove_" . $folderitem->{'directory'};
	
	$identifier = lc($identifier);

	return $identifier;
}

########################################################################
# Returning the Component for a folderitem for removefile table.
########################################################################

sub get_removefile_component
{
	my ($folderitem) = @_;

	return $folderitem->{'component'};
}

########################################################################
# Returning the FileName for a folderitem for removefile table.
########################################################################

sub get_removefile_filename
{
	my ($folderitem) = @_;

	 # return nothing: The assigned directory will be removed

	return "";
}

########################################################################
# Returning the DirProperty for a folderitem for removefile table.
########################################################################

sub get_removefile_dirproperty
{
	my ($folderitem) = @_;

	return $folderitem->{'directory'};
}

########################################################################
# Returning the InstallMode for a folderitem for removefile table.
########################################################################

sub get_removefile_installmode
{
	my ($folderitem) = @_;

	# always returning "2": The file is only removed, if the assigned
	# component is removed. Name: msidbRemoveFileInstallModeOnRemove

	return 2;
}

###########################################################################################################
# Creating the file RemoveFi.idt dynamically
# Content: 
# FileKey Component_ FileName DirProperty InstallMode
###########################################################################################################

sub create_removefile_table
{
	my ($folderitemsref, $basedir) = @_;

	my @removefiletable = ();
	
	installer::windows::idtglobal::write_idt_header(\@removefiletable, "removefile");

	# Only the directories created for the FolderItems have to be deleted
	# with the information in the table RemoveFile

	my @directorycollector = ();

	for ( my $i = 0; $i <= $#{$folderitemsref}; $i++ )
	{
		my $onelink = ${$folderitemsref}[$i];
		
		if ( $onelink->{'used'} == 0 ) { next; }
		
		if ( installer::existence::exists_in_array($onelink->{'directory'}, \@directorycollector)) { next; }

		push(@directorycollector, $onelink->{'directory'});		
		
		my %removefile = ();

		$removefile{'FileKey'} = get_removefile_filekey($onelink);
		$removefile{'Component_'} = get_removefile_component($onelink);
		$removefile{'FileName'} = get_removefile_filename($onelink);
		$removefile{'DirProperty'} = get_removefile_dirproperty($onelink);
		$removefile{'InstallMode'} = get_removefile_installmode($onelink);

		my $oneline = $removefile{'FileKey'} . "\t" . $removefile{'Component_'} . "\t" . $removefile{'FileName'} . "\t"
					. $removefile{'DirProperty'} . "\t" . $removefile{'InstallMode'} . "\n";

		push(@removefiletable, $oneline);		
	}

	# Saving the file
	
	my $removefiletablename = $basedir . $installer::globals::separator . "RemoveFi.idt";
	installer::files::save_file($removefiletablename ,\@removefiletable);
    $installer::logger::Lang->printf("Created idt file: %s\n", $removefiletablename); 
}

1;
