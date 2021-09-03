#include <array>
#include <vtkActor.h>
#include <vtkCameraOrientationWidget.h>
#include <vtkColorSeries.h>
#include <vtkColorTransferFunction.h>
#include <vtkCurvatures.h>
#include <vtkLookupTable.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>

// Here is the ComputeCurvatures definition.
//
// Includes needed by the ComputeCurvatures definition.
#include <array>
#include <map>
#include <set>
#include <string>

namespace {

/**
@class ComputeCurvatures

This class takes a vtkPolyData source and:
 - calculates Gaussian and Mean curvatures,
 - adjusts curvatures along the edges using a weighted average,
 - inserts the adjusted curvatures into the vtkPolyData source.

 Additional methods are provided for setting bounds and precision.
*/
class ComputeCurvatures
{
public:
  ComputeCurvatures() = default;
  ~ComputeCurvatures() = default;

  explicit ComputeCurvatures(vtkPolyData* source)
  {
    this->source = source;
  }

  ComputeCurvatures(vtkPolyData* source, double const& gaussEps,
                    double const& meanEps)
  {
    this->source = source;
    this->epsilons["Gauss_Curvature"] = gaussEps;
    this->epsilons["Mean_Curvature"] = meanEps;
  }

public:
  void Update();

  // Remember to run Update after these set and on/off methods.

  std::string GetCurvatureType();

  void SetCurvatureTypeToGaussian();

  void SetGaussEpsilon(double const& gauss_eps = 1.0e-8);

  void SetGaussCurvatureBounds(double const& lower = 0.0,
                               double const& upper = 0.0);
  void GaussBoundsOn();

  void GaussBoundsOff();

  void SetCurvatureTypeToMean();

  void SetMeanEpsilon(double const& gauss_eps = 1.0e-8);

  void SetMeanCurvatureBounds(double const& lower = 0.0,
                              double const& upper = 0.0);
  void MeanBoundsOn();

  void MeanBoundsOff();

private:
  void ComputeCurvatureAndFixUpBoundary();
  vtkSmartPointer<vtkPolyData> ComputeCurvature();
  std::vector<double> ExtractData(vtkPolyData* curvatureData);
  std::vector<vtkIdType> ExtractBoundaryIds();
  double ComputeDistance(vtkIdType const& ptIdA, vtkIdType const& ptIdB);
  std::set<vtkIdType> PointNeighborhood(vtkIdType const& pId);
  void UpdateCurvature();

public:
  vtkSmartPointer<vtkPolyData> source;

private:
  std::string curvatureType{"Gauss_Curvature"};
  std::map<std::string, std::vector<double>> adjustedCurvatures;
  std::map<std::string, std::vector<double>> bounds{
      {"Gauss_Curvature", {0.0, 0.0}}, {"Mean_Curvature", {0.0, 0.0}}};
  std::map<std::string, bool> boundsState{{"Gauss_Curvature", false},
                                          {"Mean_Curvature", false}};
  std::map<std::string, double> epsilons{{"Gauss_Curvature", 1.0e-8},
                                         {"Mean_Curvature", 1.0e-8}};
};

} // namespace

int main(int argc, char* argv[])
{
  vtkNew<vtkNamedColors> colors;

  // Parse command line arguments
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0]
              << " Filename(.vtp) e.g. cowHead.vtp m -20 20 16" << std::endl;
    return EXIT_FAILURE;
  }

  // Defaults
  std::array<double, 2> gaussBounds{0.0, 0.0};
  std::array<double, 2> meanBounds{0.0, 0.0};
  std::string curvature{"mean"};
  if (argc > 2)
  {
    std::string tmp = argv[2];
    if (tmp[0] == 'g' || tmp[0] == 'G')
    {
      curvature = "gauss";
    }
    else
    {
      if (tmp[0] == 'm' || tmp[0] == 'm')
      {
        curvature = "mean";
      }
      else
      {
        std::cerr << "Curvature must be either gaussian or mean." << std::endl;
        return EXIT_FAILURE;
      }
    }
  }
  if (argc > 4)
  {
    if (curvature == "gauss")
    {
      gaussBounds[0] = std::stod(argv[3]);
      gaussBounds[1] = std::stod(argv[4]);
    }
    if (curvature == "mean")
    {
      meanBounds[0] = std::stod(argv[3]);
      meanBounds[1] = std::stod(argv[4]);
    }
  }

  int scheme = 16;
  if (argc > 5)
  {
    scheme = atoi(argv[5]);
  }

  // Create a polydata
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(argv[1]);
  reader->Update();

  ComputeCurvatures cc(reader->GetOutput());
  if (curvature == "gauss")
  {
    cc.SetCurvatureTypeToGaussian();
    if (gaussBounds[0] == 0 && gaussBounds[1] == 0)
    {
      cc.GaussBoundsOff();
    }
    else
    {
        // Try these bounds: -100.0, 200.0
      cc.SetGaussCurvatureBounds(gaussBounds[0], gaussBounds[1]);
      cc.GaussBoundsOn();
    }
  }
  else
  {
    if (curvature == "mean")
    {
      cc.SetCurvatureTypeToMean();
      if (meanBounds[0] == 0 && meanBounds[1] == 0)
      {
        cc.MeanBoundsOff();
      }
      else
      {
        cc.SetMeanCurvatureBounds(meanBounds[0], meanBounds[1]);
        cc.MeanBoundsOn();
      }
    }
    else
    {
      std::cerr << "Unknown curvature" << std::endl;
      return EXIT_FAILURE;
    }
  }
  cc.Update();

  curvature = cc.GetCurvatureType();
  std::array<double, 2> scalarRange{0.0, 0.0};
  cc.source->GetPointData()
      ->GetScalars(curvature.c_str())
      ->GetRange(scalarRange.data());

  auto curvatureTitle = curvature;
  std::replace(curvatureTitle.begin(), curvatureTitle.end(), '_', '\n');

  // Uncomment the following lines if you want to write out the polydata.
  // vtkNew<vtkXMLPolyDataWriter> writer;
  // writer->SetFileName("Source.vtp");
  // writer->SetInputData(cc.source);
  // writer->SetDataModeToAscii();
  // writer->Write();

  // Build a lookup table
  vtkNew<vtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(scheme);
  std::cout << "Using color scheme #: " << colorSeries->GetColorScheme()
            << ", " << colorSeries->GetColorSchemeName() << std::endl;

  vtkNew<vtkColorTransferFunction> lut;
  lut->SetColorSpaceToHSV();

  // Use a color series to create a transfer function
  auto numColors = colorSeries->GetNumberOfColors();
  for (int i = 0; i < numColors; i++)
  {
    vtkColor3ub color = colorSeries->GetColor(i);
    double dColor[3];
    dColor[0] = static_cast<double>(color[0]) / 255.0;
    dColor[1] = static_cast<double>(color[1]) / 255.0;
    dColor[2] = static_cast<double>(color[2]) / 255.0;
    double t = scalarRange[0] +
        (scalarRange[1] - scalarRange[0]) /
            (static_cast<double>(numColors) - 1) * i;
    lut->AddRGBPoint(t, dColor[0], dColor[1], dColor[2]);
  }

  // Create a mapper and actor.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(cc.source);
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray(curvature.c_str());
  mapper->SetScalarRange(scalarRange.data());
  mapper->SetLookupTable(lut);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  auto windowWidth = 800;
  auto windowHeight = 800;

  // Create a scalar bar
  vtkNew<vtkScalarBarActor> scalarBar;
  scalarBar->SetLookupTable(mapper->GetLookupTable());
  scalarBar->SetTitle(curvatureTitle.c_str());
  scalarBar->UnconstrainedFontSizeOn();
  scalarBar->SetNumberOfLabels(5);
  scalarBar->SetMaximumWidthInPixels(windowWidth / 8);
  scalarBar->SetMaximumHeightInPixels(windowHeight / 3);

  // Create a renderer, render window, and interactor
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(windowWidth, windowHeight);
  renWin->SetWindowName("Curvatures");

  vtkNew<vtkRenderWindowInteractor> iRen;
  iRen->SetRenderWindow(renWin);
  // Important: The interactor must be set prior to enabling the widget.
  iRen->SetRenderWindow(renWin);

  vtkNew<vtkCameraOrientationWidget> camOrientManipulator;
  camOrientManipulator->SetParentRenderer(renderer);
  // Enable the widget.
  camOrientManipulator->On();

  // Add the actors to the scene
  renderer->AddActor(actor);
  renderer->AddActor2D(scalarBar);
  renderer->SetBackground(colors->GetColor3d("DarkSlateGray").GetData());

  // Render and interact
  renWin->Render();
  iRen->Start();

  return EXIT_SUCCESS;
}

// Here is the implementation for ComputeCurvatures
//
// Includes needed by the ComputeCurvatures implementation.

#include <vtkCurvatures.h>
#include <vtkDoubleArray.h>
#include <vtkFeatureEdges.h>
#include <vtkIdFilter.h>
#include <vtkIdList.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkType.h>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <numeric>
#include <vector>

namespace {

std::string ComputeCurvatures::GetCurvatureType()
{
  return this->curvatureType;
}

void ComputeCurvatures::SetCurvatureTypeToGaussian()
{
  this->curvatureType = "Gauss_Curvature";
}

void ComputeCurvatures::SetGaussEpsilon(double const& gauss_eps)
{
  this->epsilons["Gauss_Curvature"] = std::abs(gauss_eps);
}

void ComputeCurvatures::SetGaussCurvatureBounds(double const& lower,
                                                double const& upper)
{
  if (lower <= upper)
  {
    this->bounds["Gauss_Curvature"][0] = lower;
    this->bounds["Gauss_Curvature"][1] = upper;
  }
  else
  {
    this->bounds["Gauss_Curvature"][0] = upper;
    this->bounds["Gauss_Curvature"][1] = lower;
    std::cout << "SetGaussCurvatureBounds: bounds swapped since lower > upper"
              << std::endl;
  }
}

void ComputeCurvatures::GaussBoundsOn()
{
  boundsState["Gauss_Curvature"] = true;
}

void ComputeCurvatures::GaussBoundsOff()
{
  boundsState["Gauss_Curvature"] = false;
}

void ComputeCurvatures::SetCurvatureTypeToMean()
{
  this->curvatureType = "Mean_Curvature";
}

void ComputeCurvatures::SetMeanEpsilon(double const& gauss_eps)
{
  this->epsilons["Mean_Curvature"] = std::abs(gauss_eps);
}

void ComputeCurvatures::SetMeanCurvatureBounds(double const& lower,
                                               double const& upper)
{
  if (lower <= upper)
  {
    this->bounds["Mean_Curvature"][0] = lower;
    this->bounds["Mean_Curvature"][1] = upper;
  }
  else
  {
    this->bounds["Mean_Curvature"][0] = upper;
    this->bounds["Mean_Curvature"][1] = lower;
    std::cout << "SetMeanCurvatureBounds: bounds swapped since lower > upper"
              << std::endl;
  }
}

void ComputeCurvatures::MeanBoundsOn()
{
  boundsState["Mean_Curvature"] = true;
}

void ComputeCurvatures::MeanBoundsOff()
{
  boundsState["Mean_Curvature"] = false;
}

void ComputeCurvatures::Update()
{
  this->ComputeCurvatureAndFixUpBoundary();
  // Set small values to zero.
  if (this->epsilons[this->curvatureType] != 0.0)
  {
    auto eps = std::abs(this->epsilons[this->curvatureType]);
    for (size_t i = 0; i < this->adjustedCurvatures[this->curvatureType].size();
         ++i)
    {
      if (std::abs(this->adjustedCurvatures[this->curvatureType][i]) < eps)
      {
        this->adjustedCurvatures[this->curvatureType][i] = 0.0;
      }
    }
  }
  //  Set upper and lower bounds.
  if (this->boundsState[this->curvatureType])
  {
    auto lowerBound = this->bounds[this->curvatureType][0];
    for (size_t i = 0; i < this->adjustedCurvatures[this->curvatureType].size();
         ++i)
    {
      if (this->adjustedCurvatures[this->curvatureType][i] < lowerBound)
      {
        this->adjustedCurvatures[this->curvatureType][i] = lowerBound;
      }
    }
    auto upperBound = this->bounds[this->curvatureType][1];
    for (size_t i = 0; i < this->adjustedCurvatures[this->curvatureType].size();
         ++i)
    {
      if (this->adjustedCurvatures[this->curvatureType][i] > upperBound)
      {
        this->adjustedCurvatures[this->curvatureType][i] = upperBound;
      }
    }
  }
  this->UpdateCurvature();
}

void ComputeCurvatures::ComputeCurvatureAndFixUpBoundary()
{
  // Curvature as vtkPolyData.
  auto curvatureData = this->ComputeCurvature();
  // Curvature as a vector.
  auto curvature = this->ExtractData(curvatureData);
  // Ids of the boundary points.
  auto pIds = this->ExtractBoundaryIds();
  // Remove duplicate Ids.
  std::set<vtkIdType> pIdsSet(pIds.begin(), pIds.end());
  for (auto pId : pIds)
  {
    auto pIdsNeighbors = this->PointNeighborhood(pId);
    std::set<vtkIdType> pIdsNeighborsInterior;
    std::set_difference(
        pIdsNeighbors.begin(), pIdsNeighbors.end(), pIdsSet.begin(),
        pIdsSet.end(),
        std::inserter(pIdsNeighborsInterior, pIdsNeighborsInterior.begin()));
    // Compute distances and extract curvature values.
    std::vector<double> curvs;
    std::vector<double> dists;
    for (auto pIdN : pIdsNeighborsInterior)
    {
      curvs.push_back(curvature[pIdN]);
      dists.push_back(this->ComputeDistance(pIdN, pId));
    }
    std::vector<vtkIdType> nonZeroDistIds;
    for (size_t i = 0; i < dists.size(); ++i)
    {
      if (dists[i] > 0)
      {
        nonZeroDistIds.push_back(i);
      }
    }
    std::vector<double> curvsNonZero;
    std::vector<double> distsNonZero;
    for (auto i : nonZeroDistIds)
    {
      curvsNonZero.push_back(curvs[i]);
      distsNonZero.push_back(dists[i]);
    }
    // Iterate over the edge points and compute the curvature as the weighted
    // average of the neighbors.
    auto countInvalid = 0;
    auto newCurv = 0.0;
    if (curvsNonZero.size() > 0)
    {
      std::vector<double> weights;
      double sum = 0.0;
      for (auto d : distsNonZero)
      {
        sum += 1.0 / d;
        weights.push_back(1.0 / d);
      }
      for (size_t i = 0; i < weights.size(); ++i)
      {
        weights[i] = weights[i] / sum;
      }
      newCurv = std::inner_product(curvsNonZero.begin(), curvsNonZero.end(),
                                   weights.begin(), 0.0);
    }
    else
    {
      // Corner case.
      countInvalid += 1;
      newCurv = 0;
    }
    // Set the new curvature value.
    curvature[pId] = newCurv;
  }
  this->adjustedCurvatures[this->curvatureType] = curvature;
}

vtkSmartPointer<vtkPolyData> ComputeCurvatures::ComputeCurvature()
{
  vtkNew<vtkCurvatures> curvatureFilter;
  curvatureFilter->SetInputData(this->source);
  if ("Gauss_Curvature" == this->curvatureType)
  {
    curvatureFilter->SetCurvatureTypeToGaussian();
  }
  else if ("Mean_Curvature" == this->curvatureType)
  {
    curvatureFilter->SetCurvatureTypeToMean();
  }
  else
  {
    std::cerr << "Curvature type must be either Gaussian or Mean." << std::endl;
    vtkSmartPointer<vtkPolyData> ret;
    return ret;
  }
  curvatureFilter->Update();
  return curvatureFilter->GetOutput();
}

std::vector<double> ComputeCurvatures::ExtractData(vtkPolyData* curvatureData)
{
  auto array = curvatureData->GetPointData()->GetAbstractArray(
      this->curvatureType.c_str());
  auto n = curvatureData->GetNumberOfPoints();
  std::vector<double> data;
  for (auto i = 0; i < n; ++i)
  {
    data.push_back(array->GetVariantValue(i).ToDouble());
  }
  return data;
}

std::vector<vtkIdType> ComputeCurvatures::ExtractBoundaryIds()
{
  std::string name = "Ids";
  vtkNew<vtkIdFilter> idFilter;
  idFilter->SetInputData(this->source);
  idFilter->SetPointIds(true);
  idFilter->SetCellIds(false);
  idFilter->SetPointIdsArrayName(name.c_str());
  idFilter->SetCellIdsArrayName(name.c_str());
  idFilter->Update();

  vtkNew<vtkFeatureEdges> edges;

  edges->SetInputConnection(idFilter->GetOutputPort());
  edges->BoundaryEdgesOn();
  edges->ManifoldEdgesOff();
  edges->NonManifoldEdgesOff();
  edges->FeatureEdgesOff();
  edges->Update();

  auto array =
      edges->GetOutput()->GetPointData()->GetAbstractArray(name.c_str());
  auto n = edges->GetOutput()->GetNumberOfPoints();
  std::vector<vtkIdType> boundaryIds;
  for (auto i = 0; i < n; ++i)
  {
    boundaryIds.push_back(array->GetVariantValue(i).ToInt());
  }
  return boundaryIds;
}

/**
 * Extract the topological neighbors for point pId. In two steps:
 * 1) self.source.GetPointCells(pId, cellIds)
 * 2) self.source.GetCellPoints(cellId, cellPointIds) for all cellId in
 * cellIds
 */
std::set<vtkIdType> ComputeCurvatures::PointNeighborhood(vtkIdType const& pId)
{
  vtkNew<vtkIdList> cellIds;
  this->source->GetPointCells(pId, cellIds);
  std::set<vtkIdType> neighbors;
  auto n = cellIds->GetNumberOfIds();
  for (auto i = 0; i < n; ++i)
  {
    auto cellId = cellIds->GetId(i);
    vtkNew<vtkIdList> cellPointIds;
    this->source->GetCellPoints(cellId, cellPointIds);
    for (auto j = 0; j < cellPointIds->GetNumberOfIds(); ++j)
    {
      neighbors.insert(cellPointIds->GetId(j));
    }
  }
  return neighbors;
}

double ComputeCurvatures::ComputeDistance(vtkIdType const& ptIdA,
                                          vtkIdType const& ptIdB)
{
  double ptA[3]{0.0, 0.0, 0.0};
  double ptB[3]{0.0, 0.0, 0.0};
  double ptC[3]{0.0, 0.0, 0.0};
  this->source->GetPoint(ptIdA, ptA);
  this->source->GetPoint(ptIdB, ptB);
  vtkMath::Subtract(ptA, ptB, ptC);
  return vtkMath::Norm(ptC);
}

void ComputeCurvatures::UpdateCurvature()
{
  if (static_cast<size_t>(this->source->GetNumberOfPoints()) !=
      this->adjustedCurvatures[this->curvatureType].size())
  {
    std::string s = this->curvatureType;
    s += ":\nCannot add the adjusted curvatures to the source.\n";
    s += " The number of points in source does not equal the\n";
    s += " number of point ids in the adjusted curvature array.";
    std::cerr << s << std::endl;
    return;
  }
  vtkNew<vtkDoubleArray> curvatures;
  curvatures->SetName(this->curvatureType.c_str());
  for (auto curvature : this->adjustedCurvatures[this->curvatureType])
  {
    curvatures->InsertNextTuple1(curvature);
  }
  this->source->GetPointData()->AddArray(curvatures);
  this->source->GetPointData()->SetActiveScalars(this->curvatureType.c_str());
}

} // namespace
