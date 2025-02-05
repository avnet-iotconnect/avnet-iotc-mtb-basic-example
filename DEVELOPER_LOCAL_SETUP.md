## Development Setup with Git

In order to develop this project with Git:
- Clone this repo into a directory of your choosing.
- Follow the steps of the main Developer Guide, but when on the Project Creator application selection screen,
select **Browse for Application** and browse to the location of this  git repo.
- Checkmark your application that now appeared in the *Import* category.
- It is usually beneficial to rename the application to a shorter string 
to minimize potential of running into issues with long path names on Windows.
- Continue following the main guide steps, but do not compile the application yet until you have done the Library Manager step below.
- At this point, your originally cloned repo is no longer needed, so you may delete it at this time if you wish.
Your repo clone will be copied into the directory you selected in the Project Creator and you can continue 
to use this directory with Git.
- Note that you can use Git with the SDK created in `mtb_shared/avnet-iotc-mtb-sdk/<label>`
in the same way you would your application clone, with the exception that
the remote normally called "origin" will be named "cypress". This directory
can be used to make changes to the SDK and commit them to the repo.

> [!WARNING]
> Make sure to commit and push any changes to the SDK before running library manager, or invoking `make getlibs`, as it will likely reload `mtb_shared/avnet-iotc-mtb-sdk/<label>` from the git repository.


## Local Manifest Setup

If you wish to test older versions of the software,
or your newly committed branch, you will need to point Modus Toolbox Project Creator
to a custom manifest. If so, follow these steps in order to do so:

- After installing Modus Toolbox&trade; and before opening the Project Creator in steps of the main Developer Guide,
.modustoolbox directory should be created in your user directory (On Windows it is C:/Users/*your-username*/.modustoolbox)
If the directory does not exist, create the directory.
- Create a new file in that directory named *manifest.loc*.
- Add the following content (or similar) if you wish to point to an earlier release of the application, 
put your released application version in the URL format, for example:
```
https://raw.githubusercontent.com/avnet-iotconnect/avnet-iotc-mtb-super-manifest/release-v4.0.0/avnet-iotc-mtb-super-manifest-fv2.xml
```
Using the above URL is all that is needed to achieve this

### For full control, follow this section
- Add the following content if you need full control of each sample application:
```
file:///c:/mtb-manifests/avnet-iotc-mtb-super-manifest-fv2.xml
```
The *mtb-manifests* directory path can be changed to a path of your choosing where you will store the manifests on your local disk,
so use that path in the steps below.

Download the [avnet-iotc-mtb-super-manifest-fv2.xml](https://raw.githubusercontent.com/avnet-iotconnect/avnet-iotc-mtb-super-manifest/main/avnet-iotc-mtb-super-manifest-fv2.xml)
and [avnet-iotc-mtb-ce-manifest-fv2.xml](https://raw.githubusercontent.com/avnet-iotconnect/avnet-iotc-mtb-ce-manifest/main/avnet-iotc-mtb-ce-manifest-fv2.xml)
and place them your *mtb-manifests* directory.

Edit *avnet-iotc-mtb-super-manifest-fv2.xml* and replace the **app-manifest** URL with:
```xml
    <app-manifest>
      <uri>file:///c:/mtb-manifests/avnet-iotc-mtb-ce-manifest-fv2.xml</uri>
    </app-manifest>
```

Edit *avnet-iotc-mtb-ce-manifest-fv2.xml* and replace the version release tag of the application 
entries of the applications that you intend to work on with *your-branch* or *your-label* if you are testing a label.
```xml
	<versions>
        <version flow_version="2.0" tools_min_version="3.0.0" req_capabilities_per_version="bsp_gen4">
            <num>your-branch</num>
            <commit>your-branch</commit>
        </version>
    </version>
```
From now on, the Project Creator will create projects with the contents of the branch or label that you specified. 