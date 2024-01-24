import pytest

from exactextract import (
    FeatureSequentialProcessor,
    JSONFeatureSource,
    JSONWriter,
    NumPyRasterSource,
    Operation,
)


def test_valid_stat(np_raster_source):
    valid_stats = (
        "count",
        "sum",
        "mean",
        "min",
        "max",
        "minority",
        "majority",
        "variety",
        "variance",
        "stdev",
        "coefficient_of_variation",
    )
    for stat in valid_stats:
        op = Operation(stat, "test", np_raster_source)
        assert op.stat == stat


def test_field_name(np_raster_source):
    op = Operation("count", "any_field_name", np_raster_source)
    assert op.name == "any_field_name"


def test_valid_raster(np_raster_source):
    op = Operation("count", "test", np_raster_source)
    assert op.values == np_raster_source


@pytest.mark.parametrize("raster", ("invalid", None))
def test_invalid_raster(raster):
    with pytest.raises(TypeError):
        Operation("count", "test", raster)  # type: ignore


def test_valid_weights(np_raster_source):
    rs1 = np_raster_source
    rs2 = np_raster_source

    op = Operation("count", "test", rs1, None)
    assert op.values == rs2
    assert op.weights is None

    op = Operation("weighted_mean", "test", rs1, rs2)
    assert op.values == rs1
    assert op.weights == rs2


def test_invalid_weights(np_raster_source):
    with pytest.raises(TypeError):
        Operation("count", "test", np_raster_source, "invalid")  # type: ignore


def test_stat_arguments(np_raster_source):
    op = Operation("quantile", "test", np_raster_source, None, {"q": 0.333})
    assert op.values == np_raster_source


import numpy as np


@pytest.mark.parametrize(
    "stat,typ,dtype",
    (
        ("count", float, None),
        ("variety", int, None),
        ("mode", int, None),
        ("cell_id", np.ndarray, np.int64),
        ("values", np.ndarray, np.int32),
        ("center_x", np.ndarray, np.float64),
        ("coverage", np.ndarray, np.float64),
    ),
)
def test_result_type(np_raster_source, stat, typ, dtype):
    raster_source = NumPyRasterSource(np.arange(100, dtype=np.int32).reshape(10, 10))

    op = Operation(stat, "test", raster_source)

    features = JSONFeatureSource(
        {
            "geometry": {
                "type": "Polygon",
                "coordinates": [[[0, 0], [10, 0], [10, 10], [0, 0]]],
            }
        }
    )
    writer = JSONWriter()
    fsp = FeatureSequentialProcessor(features, writer, [op])
    fsp.process()
    writer.finish()

    result = writer.features()[0]["properties"]["test"]

    assert op.result_type == typ
    assert type(result) == typ

    if type(result) is np.ndarray:
        assert result.dtype == dtype
